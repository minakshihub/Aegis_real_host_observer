#pragma once
#include "vfs_types.hpp"
#include "ring_buffer.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <array>
#include <random>

struct SessionState {
    uint64_t secret_key = 0; 
    std::atomic<uint64_t> last_time_ns{0};
    std::atomic<bool> is_currently_anomalous{false}; 
    uint64_t last_action_hash = 0;
    uint64_t prev_action_hash = 0; 
    uint32_t last_prompt_tokens = 0;
    uint32_t prev_prompt_tokens = 0; 
    uint8_t  last_payload_bucket = 0;
    uint8_t  oscillation_streak = 0; 
    uint8_t  current_trap_streak = 0;
    uint8_t  current_zombie_streak = 0;
    uint8_t  current_phantom_strikes = 0;
    uint8_t  current_read_streak = 0;
    uint64_t text_only_start_ns = 0;
    uint32_t session_start_tokens = 0;
    uint32_t session_action_count = 0;
    std::array<DenseVfsEvent, 16> history_buffer = {};
    uint8_t buffer_idx = 0;
    uint8_t buffer_count = 0; 
};

class PassiveObserver {
private:
    RingBuffer& queue;
    std::unique_ptr<SessionState[]> registry;
    std::atomic<bool> watchdog_running;
    std::thread watchdog;
    std::atomic<uint16_t> next_agent_idx{0}; 
    std::mt19937_64 rng; 

    const uint64_t VELOCITY_SPIKE_NS   = 50ULL * 1000000ULL;   
    const uint64_t IDLE_TIMEOUT_NS     = 60ULL * 1000000000ULL; 
    const uint8_t  READ_SPIRAL_LIMIT   = 10; 

    void watchdog_loop();
    void trigger_sudden_death_flush(uint16_t memory_index, uint64_t current_time_ns);

public:
    PassiveObserver(RingBuffer& q);
    ~PassiveObserver();

    uint64_t register_agent();
    uint16_t log_telemetry(uint64_t secure_agent_token, uint64_t action_hash, uint32_t prompt_tokens, uint8_t action_status, uint32_t app_latency_ms, uint32_t payload_tokens);
};