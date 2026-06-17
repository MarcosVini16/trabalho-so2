#pragma once
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdint>
#include <iostream>

#include "../../kernel/position_ioctl.h"

class Position {
public:
    static uint8_t quadrant() {
        int fd = open(POSITION_DEVICE_PATH, O_RDONLY);
        if (fd < 0) {
            std::cerr << "[Position] erro abrindo "
                      << POSITION_DEVICE_PATH << "\n";
            return 0;
        }

        uint8_t q = 0;
        if (ioctl(fd, POSITION_GET_QUADRANT, &q) < 0) {
            std::cerr << "[Position] ioctl falhou\n";
            close(fd);
            return 0;
        }

        close(fd);
        return q;
    }
};