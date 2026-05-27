#pragma once
#include <fstream>
#include <cstdint>

class Position {
public:
    static uint8_t quadrant() {
        std::ifstream f("/proc/position");
        if (!f) return 255;
        unsigned int q = 0;
        f >> q;
        return static_cast<uint8_t>(q);
    }
};