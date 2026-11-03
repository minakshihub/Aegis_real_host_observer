#pragma once
#include <cstdint>

// --- AEGIS 16-BIT DIAGNOSTIC DICTIONARY ---
const uint16_t STATUS_HEALTHY       = 0;
const uint16_t LOOP_UNRESOLVED_TRAP = 1 << 0;  // 1
const uint16_t COGNITIVE_STALL      = 1 << 1;  // 2
const uint16_t VELOCITY_SPIKE       = 1 << 2;  // 4
const uint16_t APP_IDLE_STALL       = 1 << 3;  // 8
const uint16_t CONTEXT_HEMORRHAGE   = 1 << 4;  // 16
const uint16_t ACTION_THRASHING     = 1 << 5;  // 32
const uint16_t ZOMBIE_STREAK        = 1 << 6;  // 64
const uint16_t PHANTOM_STRIKEOUT    = 1 << 7;  // 128
const uint16_t SLOW_BOIL_CREEP      = 1 << 8;  // 256 (NEW)
const uint16_t READ_ONLY_SPIRAL     = 1 << 9;  // 512 (NEW)

// Perfectly packed to exactly 32 bytes (Zero Padding Required)
struct DenseVfsEvent {
    uint64_t timestamp_ns;   // 8 bytes
    uint64_t action_hash;    // 8 bytes
    uint32_t session_hash;   // 4 bytes
    uint32_t prompt_tokens;  // 4 bytes
    uint32_t time_delta_ms;  // 4 bytes
    uint16_t anomaly_flags;  // 2 bytes (Upgraded)
    uint8_t  action_status;  // 1 byte
    uint8_t  payload_bucket; // 1 byte
};