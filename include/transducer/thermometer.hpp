#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"
#include <random>

/*
 * Classe específica para o transdutor de termômetro, que sensoria a temperatura ambiente.
 */
class Thermometer : public Transducer<SmartData::Unit::Temperature> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Temperature>::Type; // Tipo de dado numérico correspondente à unidade de temperatura
    static Value sense() {
        static bool seeded = false;
        if (!seeded) {
            srand(time(nullptr) ^ (SmartData::Unit::Temperature & 0xFFFF));
            seeded = true;
        }
        // Thermometer - temperatura entre 280K e 320K (7°C a 47°C)
        return 280.0 + (rand() % 400) / 10.0;
    }
};