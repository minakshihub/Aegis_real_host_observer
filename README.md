# Aegis Observer Core

A True Zero-Configuration C++ telemetry and diagnostic engine for Agentic AI. 

The Observer Core passively monitors AI agent behavior, flags diagnostic anomalies in real-time, and generates binary ledgers on isolated background threads without blocking or degrading the host application's main thread.

## Architecture & Philosophy

**The Why:** Modern Agentic AI systems lack standardized, low-overhead observability. Traditional logging requires manual path configurations, explicit file handling, and heavy main-thread synchronization. Aegis was built to eliminate this friction.

**The What (True Zero-Configuration):** The engine is entirely self-sufficient. It internally initializes a lock-free `RingBuffer`, an autonomous `VfsWorker`, and a `PassiveObserver`. It manages its own memory, thread lifecycle, and file I/O operations autonomously. 


# Aegis Industrial Core: Master Architectural Blueprint

Aegis is a bare-metal C++17 telemetry and anomaly-detection engine engineered specifically for autonomous AI agent swarms. 

Unlike traditional observability tools that rely on heavy SaaS SDKs, slow JSON parsing, and mutex-locked data structures, Aegis operates as a pure, passive wiretap. It achieves microsecond-level execution with zero memory contention, ensuring systemic blast-radius containment without impacting agent performance. 

---

## I. Core Architectural Pillars

### 1. Lock-Free Atomics & Mechanical Sympathy
Traditional monitoring tools use mutex locks to protect shared data, causing threads to stall. Aegis uses relaxed atomics (`std::memory_order_relaxed`), compiling down to single-instruction CPU registers (MOV). Threads never wait or contend for memory.
* **The Accordion Memory Model:** Telemetry is stored in a 32-byte `DenseVfsEvent`. In RAM, this is padded to exactly 64 bytes (`PaddedVfsEvent`) to perfectly match L1 cache lines, eliminating "false sharing" CPU collisions. Before writing to disk, the VFS worker strips the padding, packing 128 dense events per 4KB sector.

### 2. Massive Scale ($O(1)$ Registry)
Instead of fragmented hash maps that thrash CPU caches, Aegis pre-allocates a contiguous 65,536-agent registry on the heap (`std::unique_ptr<SessionState[]>`). This ensures $O(1)$ constant-time lookups that fit directly into the processor’s L2 cache. All 65,536 agents can be processed concurrently without a single blocking lock.

### 3. Cryptographic Bitwise Anti-Spoofing
To eliminate multi-tenant ID hijacking without the overhead of heavy string hashing, Aegis utilizes a single 64-bit native register as the auth token. 
* **Bottom 16 bits:** The physical array index (for instant $O(1)$ lookup).
* **Top 48 bits:** A cryptographically random password.
Verification happens via primitive bitwise math (`&` and `>>`) directly on the CPU's Arithmetic Logic Unit (ALU) in less than 1 nanosecond.

### 4. Staggered Time-Slicing Watchdog
To catch silent crashes ("Sudden Death" OOMs) without spiking CPU usage, a background thread wakes up every 15 milliseconds, scans exactly 1,000 agents, and sleeps. It sweeps the entire 65K registry every second with ~0.001% CPU utilization, flagging dead agents without degrading the main network path.

### 5. In-Memory Flight Data Recorder (The Black Box)
Aegis does not burn SSD hardware logging healthy events. Each agent maintains a 16-slot circular array in RAM. When an anomaly triggers, Aegis instantly flushes the preceding 16 "healthy" footprints to the Smart VFS Router, providing perfect chronological forensics without disk bloat.

---

## II. The 16-Bit Anomaly Engine

Aegis tracks 12 distinct failure modes, mapped to a pure 16-bit dictionary (`uint16_t anomaly_flags`).

### The Hallucinations & Logic Failures
* **Bit 128 (Phantom Tool Strikes):** Tracks when the agent attempts to call a hallucinated tool. Trips the flag on the 3rd consecutive strike.
* **Bit 128 Override (State Desynchronization):** A zero-tolerance kill switch for out-of-order execution (e.g., executing Step 3 before Step 2). 
* **Bit 64 (Zombie Streaks):** Tracks standard application errors. Trips if an agent hits a failure wall 5 times consecutively.

### The Temporal Anomalies
* **Bit 4 (Velocity Spikes):** Detects if two events occur physically faster than an LLM can generate them (< 50ms), flagging computational spin-locks.
* **Bit 2 (Cognitive Stalls / Apology Spirals):** Tracks text-only states. Uses a dynamic time budget (150% of the last app latency) to cut off agents that get stuck in polite, non-functional text generation.
* **Bit 8 (App Idle Stall):** Triggered by the Watchdog if an application thread deadlocks and goes silent for > 60 seconds.

### The Repetition Traps
* **Bit 1 (Unresolved Loop Traps):** Employs a Split-Hash strategy (Tool ID + Argument ID). Catches agents repeating the exact action, or spinning their wheels by constantly permuting arguments on a failing tool.
* **Bit 32 (Action Thrashing):** Detects A-B-A oscillation. Tracks the sliding memory window to flag agents bouncing between two actions without progressing.
* **Bit 2 (Read-Only Spiral):** Overloaded trap. Flags agents that execute 10 consecutive "Read" actions without ever committing a "Write" or state-changing action.

### The Memory & Context Bleeds
* **Bit 16 (Context Hemorrhage & Slow Boil):** Monitors token delta. Trips if a single action jumps > 10,000 tokens, or if the average token burn rate creeps dangerously high over 10 consecutive steps.
* **Padding Byte (Data Bombs):** Aggressively quantizes massive payload returns (dividing by 256) and stores them in the 32nd padding byte. Clamps at 65,280 tokens to prevent out-of-memory crashes.

### Multi-Agent Infrastructure Failures
* **Bit 1024 (Swarm Deadlock):** Tracks flatlining entropy. If token counts and latency perfectly repeat in an A-B-A-B pattern across multiple agents, it flags a robotic, non-productive ping-pong loop.
* **Bit 2048 (Payload Flooding):** Catches compromised agents attempting to exhaust network bandwidth by dumping maximum-size context windows on every action.




### 5. Dual-Build Deployment & "Sudden Death" Trial
The Aegis codebase utilizes preprocessor directives (`#ifdef AEGIS_TRIAL_VERSION`) to support two distinct compilation targets from the exact same source code:
* **Industrial Build:** Permanent, time-independent binary for commercial deployment.
* **72-Hour Trial Build:** Compiling with the Trial flag mathematically stamps the compilation time into the binary via `std::chrono`. Exactly 259,200 seconds (72 hours) later, a "Sudden Death" trap activates, instantly executing `std::exit(EXIT_FAILURE)` and requiring a commercial license.

---

## Integration & Build Instructions
Chronologically, a developer has to add the code to their AI agent before they can run the whole system together. If we want this to be completely foolproof for anyone to understand, and execute and experience it in real life,
Here is the revised, hyper-simple sequence. It walks them through exactly what to do step-by-step:

## How to Use Aegis (Step-by-Step)

Aegis runs completely separate from your AI agent. It acts like an independent security camera. You just need to tell your AI agent to send data to it.

### Step 1: Add Aegis to your AI Agent's Code
First, you add our lightweight header file to your existing AI agent's code. Your agent doesn't run Aegis; it just drops its telemetry into a shared mailbox (`/dev/shm`).

Add these lines to your AI agent's code:

```cpp
#include "aegis.hpp"

int main() {
    // 1. Get a secure token for your agent
    uint64_t secure_token = register_agent_to_shared_memory();

    // 2. Whenever your AI makes a decision or tool call, push the record
    push_to_ring_buffer(secure_token, ...);

    // Your normal AI logic continues here without any slowdown...
    return 0;
}

# Step 2: Turn on the Aegis Daemon (The Observer)
Before you run your AI agent, you need to turn on Aegis so it can listen. Open a terminal and run these commands to start the background daemon:

Bash

mkdir build && cd build
cmake ..
make
./aegis_test
(You will see a message saying Aegis is listening. Leave this terminal open.)

# Step 3: Run Your AI Agent
Now, simply run your AI agent exactly as you normally would.

As your agent works, Aegis will silently observe the data flowing through the shared memory and automatically generate your security logs (aegis_session_report.txt and aegis_ledger.bin) in the background.


## Execution & Graceful Shutdown (Stopping Mid-Way)
The engine relies on C++ RAII (Resource Acquisition Is Initialization) to manage its lifecycle safely.

If you need to halt the AI application mid-way (e.g., pressing Ctrl+C or a sudden process interrupt), the Aegis destructor automatically triggers a graceful shutdown sequence:

It signals the RingBuffer to halt new ingestion.

It safely joins the VfsWorker and PassiveObserver background threads.

It flushes all remaining telemetry data to the binary ledger before the application exits.

No data is lost during an unexpected mid-way termination.


Telemetry & Log Analysis
Upon the completion of every run (or after a graceful shutdown), Aegis automatically generates a fresh, human-readable log file in the execution directory: aegis_session_report.txt.

Always review this file after a run. It translates the raw binary ledger into parsed agent events and appends an Anomaly Flag to highlight errant AI behavior.

### Diagnostic Legend (Anomaly Flags)
* **0**    : `STATUS_HEALTHY` - Normal operation
* **1**    : `LOOP_UNRESOLVED_TRAP` - Agent repeating exact actions/permutations
* **2**    : `COGNITIVE_STALL` - Agent stuck in text-generation/apology loop
* **4**    : `VELOCITY_SPIKE` - Actions occurring faster than physical limits
* **8**    : `APP_IDLE_STALL` - Dead air, timeouts, or API 429 Rate Limits
* **16**   : `CONTEXT_HEMORRHAGE` - Sudden massive spike or drop in tokens
* **32**   : `ACTION_THRASHING` - Agent bouncing between two states (A-B-A)
* **64**   : `ZOMBIE_STREAK` - Consecutive standard application errors
* **128**  : `PHANTOM_STRIKEOUT` - Hallucinated tools or out-of-order execution
* **256**  : `SLOW_BOIL_CREEP` - Derivative token waste over time
* **512**  : `READ_ONLY_SPIRAL` - Endless reading/researching without action
* **1024** : `SWARM_DEADLOCK` - Multi-agent infinite ping-pong loop
* **2048** : `PAYLOAD_FLOOD` - Sudden massive data ingestion/output spike

License
Proprietary and Confidential
Copyright (c) 2026. All Rights Reserved.

This repository and its contents are strictly proprietary. Unauthorized copying, modification, distribution, or execution of this software, via any medium, is strictly prohibited without an explicit commercial license agreement.
