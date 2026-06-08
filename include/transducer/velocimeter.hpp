#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"

/*
 * Classe específica para o transdutor de velocímetro, que sensoria a velocidade do veículo.
 */
class Velocimeter : public Transducer<SmartData::Unit::Velocity> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Velocity>::Type; // Tipo de dado numérico correspondente à unidade de velocidade
    static Value sense() {
        static bool seeded = false;
        if (!seeded) {
            srand(time(nullptr) ^ (SmartData::Unit::Velocity & 0xFFFF));
            seeded = true;
        }
        // Velocimeter - velocidade entre 0 e 30 m/s (0 a 108 km/h)
        return (rand() % 300) / 10.0;
    }
};