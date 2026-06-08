#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"
#include <random>

/*
 * Classe específica para o transdutor de acelerômetro, que sensoria a aceleração do veículo.
 */
class Accelerometer : public Transducer<SmartData::Unit::Acceleration> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Acceleration>::Type; // Tipo de dado numérico correspondente à unidade de aceleração
    static Value sense() {
        static bool seeded = false;
        if (!seeded) {
            srand(time(nullptr) ^ (SmartData::Unit::Acceleration & 0xFFFF));
            seeded = true;
        }
        // aceleração entre 0 e 20 m/s²
        return (rand() % 200) / 10.0;
    }
};