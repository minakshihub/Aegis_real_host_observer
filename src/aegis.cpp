#include "aegis.hpp"
#include "ring_buffer.hpp"
#include "vfs_worker.hpp"
#include "passive_observer.hpp"
#include <chrono>
#include <iostream>
#include <cstdlib>

Aegis::Aegis() {
#ifdef AEGIS_TRIAL_VERSION
    auto now = std::chrono::system_clock::now().time_since_epoch();
    long long current_time = std::chrono::duration_cast<std::chrono::seconds>(now).count();
    long long expiry_time = AEGIS_BUILD_TIME + 259200; // 72 hours (72 * 60 * 60)
    
    if (current_time > expiry_time) {
        std::cerr << "\n[Aegis Engine] FATAL: 72-Hour Trial Period has expired." << std::endl;
        std::cerr << "Please contact the vendor for a commercial license." << std::endl;
        std::exit(EXIT_FAILURE); // Instantly kills the client's application
    }
#endif
    
    // We wire up the complex internals so the client doesn't have to
    queue = std::make_unique<RingBuffer>(1024);
    worker = std::make_unique<VfsWorker>(*queue);
    observer = std::make_unique<PassiveObserver>(*queue);
}

Aegis::~Aegis() {
    // Memory is freed and threads are joined automatically in the correct order
}

uint64_t Aegis::register_agent() {
    return observer->register_agent();
}

uint16_t Aegis::log(uint64_t secure_token, uint64_t action_hash, uint32_t prompt_tokens, 
                    uint8_t action_status, uint32_t app_latency_ms, uint32_t payload_tokens) {
    return observer->log_telemetry(secure_token, action_hash, prompt_tokens, 
                                   action_status, app_latency_ms, payload_tokens);
}