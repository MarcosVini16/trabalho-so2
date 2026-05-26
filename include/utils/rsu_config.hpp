#pragma once
#include "../ethernet.hpp"
#include <cstdint>

/*
 * Tabela centralizada de RSUs.
 * Para alterar qual MAC corresponde a qual quadrante,
 * edite apenas este arquivo — o resto da pilha usa
 * mac_for_quadrant() / quadrant_for_mac() em vez de
 * derivar o quadrante do último byte do MAC.
 */
namespace RSUConfig {

    // Altere aqui para remapear quadrante ↔ MAC sem
    // tocar em RSU, TimeClient ou Component.
    static constexpr Ethernet::Address MACS[4] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0xFC}, // Q0
        {0x00, 0x00, 0x00, 0x00, 0x00, 0xFD}, // Q1
        {0x00, 0x00, 0x00, 0x00, 0x00, 0xFE}, // Q2
        {0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}, // Q3
    };

    inline Ethernet::Address mac_for_quadrant(uint8_t q) {
        return MACS[q & 0x3];
    }

    // Retorna 0xFF se o MAC não for de nenhuma RSU conhecida.
    inline uint8_t quadrant_for_mac(const Ethernet::Address& mac) {
        for (uint8_t i = 0; i < 4; i++) {
            if (MACS[i] == mac) return i;
        }
        return 0xFF;
    }

    inline bool is_rsu_mac(const Ethernet::Address& mac) {
        return quadrant_for_mac(mac) != 0xFF;
    }
}