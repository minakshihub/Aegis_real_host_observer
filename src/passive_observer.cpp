#include "passive_observer.hpp"
#include <chrono>
#include <algorithm>

PassiveObserver::PassiveObserver(RingBuffer& q) : 
    queue(q), 
    registry(std::make_unique<SessionState[]>(65536)), 
    watchdog_running(true) 
{
    rng.seed(std::chrono::system_clock::now().time_since_epoch().count());
    watchdog = std::thread(&PassiveObserver::watchdog_loop, this);
}

PassiveObserver::~PassiveObserver() {
    watchdog_running.store(false, std::memory_order_relaxed);
    if (watchdog.joinable()) watchdog.join();
}

uint64_t PassiveObserver::register_agent() {
    uint16_t idx = next_agent_idx.fetch_add(1, std::memory_order_relaxed);
    uint64_t secret = rng() & 0xFFFFFFFFFFFF; 
    registry[idx].secret_key = secret;
    return (secret << 16) | idx; 
}

void PassiveObserver::watchdog_loop() {
    uint16_t bookmark = 0;
    const uint16_t batch_size = 1000;
    
    while (watchdog_running.load(std::memory_order_relaxed)) {
        auto now = std::chrono::system_clock::now().time_since_epoch();
        uint64_t current_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
        uint16_t end_idx = std::min<uint16_t>(bookmark + batch_size, 65535);
        
        for (uint16_t i = bookmark; i < end_idx; ++i) {
            uint64_t last_time = registry[i].last_time_ns.load(std::memory_order_relaxed);
            if (last_time > 0 && !registry[i].is_currently_anomalous.load(std::memory_order_relaxed)) {
                if (current_time_ns - last_time > IDLE_TIMEOUT_NS) {
                    trigger_sudden_death_flush(i, current_time_ns);
                }
            }
        }
        bookmark = (end_idx == 65535) ? 0 : end_idx;
        std::this_thread::sleep_for(std::chrono::milliseconds(15)); 
    }
}

void PassiveObserver::trigger_sudden_death_flush(uint16_t memory_index, uint64_t current_time_ns) {
    SessionState& state = registry[memory_index];
    state.is_currently_anomalous.store(true, std::memory_order_relaxed);
    
    uint8_t read_start = (state.buffer_count == 16) ? state.buffer_idx : 0;
    for (uint8_t i = 0; i < state.buffer_count; i++) {
        uint8_t read_index = (read_start + i) % 16;
        queue.push(state.history_buffer[read_index]); 
    }
    state.buffer_count = 0;

    DenseVfsEvent autopsy_event = {};
    autopsy_event.timestamp_ns = current_time_ns;
    autopsy_event.session_hash = (state.secret_key << 16) | memory_index;
    autopsy_event.anomaly_flags = 8; 
    queue.push(autopsy_event);
}

uint16_t PassiveObserver::log_telemetry(uint64_t secure_agent_token, uint64_t action_hash, uint32_t prompt_tokens, uint8_t action_status, uint32_t app_latency_ms, uint32_t payload_tokens) {
    uint16_t memory_index = secure_agent_token & 0xFFFF; 
    uint64_t provided_password = secure_agent_token >> 16; 
    SessionState& state = registry[memory_index];

    if (state.secret_key != provided_password) return 0; 

    auto now = std::chrono::system_clock::now().time_since_epoch();
    uint64_t current_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

    uint64_t previous_time = state.last_time_ns.load(std::memory_order_relaxed);
    if (current_time_ns < previous_time) current_time_ns = previous_time + 1; 

    DenseVfsEvent event = {};
    event.timestamp_ns = current_time_ns;
    event.session_hash = secure_agent_token; 
    event.action_hash = action_hash;
    event.prompt_tokens = prompt_tokens;
    event.action_status = action_status;
    event.anomaly_flags = 0;

    if (payload_tokens >= 65280) event.payload_bucket = 255; 
    else event.payload_bucket = static_cast<uint8_t>(payload_tokens >> 8); 

    if (previous_time != 0) {
        uint64_t time_gap_ns = current_time_ns - previous_time;
        event.time_delta_ms = time_gap_ns / 1000000;

        if (action_hash == state.prev_action_hash && prompt_tokens == state.prev_prompt_tokens && action_hash != state.last_action_hash) {
            state.oscillation_streak++;
            if (state.oscillation_streak >= 3) event.anomaly_flags |= 1024; 
        } else state.oscillation_streak = 0;

        if (event.payload_bucket == 255 || (state.last_payload_bucket > 0 && event.payload_bucket > state.last_payload_bucket + 50)) {
            event.anomaly_flags |= 2048; 
        }

        if (time_gap_ns < VELOCITY_SPIKE_NS) event.anomaly_flags |= 4; 
    }
    
    state.prev_action_hash = state.last_action_hash;
    state.last_action_hash = action_hash;
    state.prev_prompt_tokens = state.last_prompt_tokens;
    state.last_prompt_tokens = prompt_tokens;
    state.last_payload_bucket = event.payload_bucket;
    
    state.last_time_ns.store(current_time_ns, std::memory_order_relaxed);

    if (event.anomaly_flags == 0) {
        state.is_currently_anomalous.store(false, std::memory_order_relaxed);
        state.history_buffer[state.buffer_idx] = event;
        state.buffer_idx = (state.buffer_idx + 1) % 16;
        if (state.buffer_count < 16) state.buffer_count++;
    } else {
        if (!state.is_currently_anomalous.load(std::memory_order_relaxed)) {
            uint8_t read_start = (state.buffer_count == 16) ? state.buffer_idx : 0;
            for (uint8_t i = 0; i < state.buffer_count; i++) {
                uint8_t read_index = (read_start + i) % 16;
                queue.push(state.history_buffer[read_index]); 
            }
            state.is_currently_anomalous.store(true, std::memory_order_relaxed);
            state.buffer_count = 0; 
        }
        queue.push(event); 
    }

    return event.anomaly_flags; 
}