#pragma once

struct Stats {
    int ptp_sync_ok = 0;
    int ptp_sync_timeout = 0;
    long long last_offset_ns = 0;
};

extern Stats g_stats;