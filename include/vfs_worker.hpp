#pragma once
#include "ring_buffer.hpp"
#include "vfs_types.hpp"
#include <thread>
#include <atomic>
#include <fstream>
#include <iomanip>
#include <chrono>

class VfsWorker {
private:
    RingBuffer& queue;
    std::atomic<bool> running;
    std::thread worker_thread;
    
    std::ofstream bin_file;
    std::ofstream txt_file;
    uint64_t event_counter = 0;

    // Helper to convert nanoseconds into human-readable time
    std::string format_timestamp(uint64_t ns) {
        auto duration = std::chrono::nanoseconds(ns);
        std::chrono::system_clock::time_point tp(std::chrono::duration_cast<std::chrono::system_clock::duration>(duration));
        std::time_t t = std::chrono::system_clock::to_time_t(tp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration) % 1000;
        
        std::tm tm = *std::localtime(&t);
        char buffer[32];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
        
        return std::string(buffer) + "." + std::to_string(ms.count());
    }

    void write_footer() {
        txt_file << "\n============================================\n"
                 << "       AEGIS DIAGNOSTIC LEGEND (FLAGS)\n"
                 << "============================================\n"
                 << "0    : STATUS_HEALTHY - Normal operation\n"
                 << "1    : LOOP_UNRESOLVED_TRAP - Agent repeating exact actions/permutations\n"
                 << "2    : COGNITIVE_STALL - Agent stuck in text-generation/apology loop\n"
                 << "4    : VELOCITY_SPIKE - Actions occurring faster than physical limits\n"
                 << "8    : APP_IDLE_STALL - Dead air, timeouts, or API 429 Rate Limits\n"
                 << "16   : CONTEXT_HEMORRHAGE - Sudden massive spike or drop in tokens\n"
                 << "32   : ACTION_THRASHING - Agent bouncing between two states (A-B-A)\n"
                 << "64   : ZOMBIE_STREAK - Consecutive standard application errors\n"
                 << "128  : PHANTOM_STRIKEOUT - Hallucinated tools or out-of-order execution\n"
                 << "256  : SLOW_BOIL_CREEP - Derivative token waste over time\n"
                 << "512  : READ_ONLY_SPIRAL - Endless reading/researching without action\n"
                 << "1024 : SWARM_DEADLOCK - Multi-agent infinite ping-pong loop\n"
                 << "2048 : PAYLOAD_FLOOD - Sudden massive data ingestion/output spike\n"
                 << "============================================\n";
    }

    void process_loop() {
        DenseVfsEvent event;
        while (running.load(std::memory_order_relaxed)) {
            while (queue.pop(event)) {
                event_counter++;
                
                // 1. Write raw binary for the machine
                bin_file.write(reinterpret_cast<const char*>(&event), sizeof(DenseVfsEvent));
                
                // 2. Generate formatted text for the human engineer
                txt_file << "Event #" << event_counter 
                         << " | Agent: " << event.session_hash 
                         << " | Time: " << format_timestamp(event.timestamp_ns)
                         << " | Tokens: " << event.prompt_tokens 
                         << " | Anomaly Flag: " << event.anomaly_flags 
                         << " | Action Status: " << static_cast<int>(event.action_status) 
                         << " | Payload Bucket: " << static_cast<int>(event.payload_bucket) << "\n";
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

public:
    VfsWorker(RingBuffer& q) : queue(q), running(true) {
        bin_file.open("aegis_ledger.bin", std::ios::binary | std::ios::app);
        txt_file.open("aegis_session_report.txt", std::ios::out | std::ios::trunc);
        
        txt_file << "--- Aegis Session Report ---\n";
        worker_thread = std::thread(&VfsWorker::process_loop, this);
    }

    ~VfsWorker() {
        running.store(false, std::memory_order_relaxed);
        if (worker_thread.joinable()) worker_thread.join();
        
        // Drain any remaining events before closing
        DenseVfsEvent event;
        while (queue.pop(event)) {
            event_counter++;
            bin_file.write(reinterpret_cast<const char*>(&event), sizeof(DenseVfsEvent));
            txt_file << "Event #" << event_counter << " | Agent: " << event.session_hash 
                     << " | Time: " << format_timestamp(event.timestamp_ns)
                     << " | Tokens: " << event.prompt_tokens << " | Anomaly Flag: " << event.anomaly_flags 
                     << " | Action Status: " << static_cast<int>(event.action_status) 
                     << " | Payload Bucket: " << static_cast<int>(event.payload_bucket) << "\n";
        }

        // Print the legend automatically upon session termination
        write_footer();

        bin_file.close();
        txt_file.close();
    }
};