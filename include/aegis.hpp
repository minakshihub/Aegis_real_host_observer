#pragma once
#include <cstdint>
#include <string>
#include <memory>

// Forward declarations to completely hide our internal classes from the client
class RingBuffer;
class VfsWorker;
class PassiveObserver;

class Aegis {
private:
    std::unique_ptr<RingBuffer> queue;
    std::unique_ptr<VfsWorker> worker;
    std::unique_ptr<PassiveObserver> observer;

public:
    // 1. The Constructor :One line to start the entire engine
    Aegis();
    
    // 2. The Destructor : safely flushes data and closes threads automatically
    ~Aegis();

    // 3. Register a new agent
    uint64_t register_agent();

    // 4. Log a step
    uint16_t log(uint64_t secure_token, uint64_t action_hash, uint32_t prompt_tokens, 
                 uint8_t action_status, uint32_t app_latency_ms, uint32_t payload_tokens);
};