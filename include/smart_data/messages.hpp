#pragma once
#include <cstdint>

enum MsgKind : uint8_t { INTEREST = 0, RESPONSE = 1 };

struct InterestMsg {
    MsgKind  kind = INTEREST;
    uint8_t  origin;
    uint64_t timestamp;
    uint32_t type;
    uint64_t period;
} __attribute__((packed));

struct ResponseMsg {
    MsgKind  kind = RESPONSE;
    uint8_t  origin;
    uint64_t timestamp;
    uint32_t type;
    double   value;
} __attribute__((packed));