#pragma once
#include <cstdint>

struct PTPFrame {
    uint8_t message_type; // 0 = Req, 1 = Resp, 2 = Delay_Req, 3 = Delay_Resp
    uint64_t timestamp; // Timestamp (T2 para Resp, T4 para Delay_Resp)

    PTPFrame() : message_type(0), timestamp(0) {}
    PTPFrame(uint8_t type, uint64_t ts) : message_type(type), timestamp(ts) {}
} __attribute__((packed));