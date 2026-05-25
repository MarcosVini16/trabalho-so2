#pragma once
#include "../utils/position.hpp"
#include "../ethernet.hpp"

/*
 * Gerencia a mobilidade do veículo — monitora o quadrante atual
 * e determina qual RSU deve ser usada para sincronização.
 */
class Mobility {
public:
    Mobility() : _quadrant(Position::quadrant()) {}

    // retorna o quadrante atual
    uint8_t quadrant() const { return _quadrant; }

    // verifica se o quadrante mudou e atualiza
    // retorna true se houve mudança
    bool update() {
        uint8_t q = Position::quadrant();
        if (q != _quadrant) {
            _quadrant = q;
            return true;
        }
        return false;
    }

    // retorna o MAC da RSU do quadrante atual
    // FC=0, FD=1, FE=2, FF=3
    Ethernet::Address rsu_mac() const {
        Ethernet::Address mac{};
        mac.bytes[5] = 0xFC + _quadrant;
        return mac;
    }

private:
    uint8_t _quadrant;
};