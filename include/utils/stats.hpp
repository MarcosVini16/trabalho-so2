#pragma once

struct Stats {
    // stats do ptp
    int ptp_sync_ok = 0;
    int ptp_sync_timeout = 0;
    long long last_offset_ns = 0;

    // stats do quadrante
    uint32_t q_count[4] = {0, 0, 0, 0};
    uint8_t  first_quadrant = 0xFF;
    uint8_t  last_quadrant  = 0xFF;
};

extern Stats g_stats;