#pragma once

struct Stats {
    int ptp_sync_ok = 0;
    int ptp_sync_timeout = 0;
    long long last_offset_ns = 0;

    int first_quadrant = -1;
    int last_quadrant = -1;
    int quadrant_count[4] = {0, 0, 0, 0};
};

extern Stats g_stats;