// include/utils/position.hpp
#pragma once
#include <fstream>
#include <cstdint>

class Position {
public:
    static uint8_t quadrant() {
        std::ifstream f("/proc/position");
        if (!f) return 0;
        unsigned int q = 0;
        f >> q;
        return q & 0x3;
    }
};