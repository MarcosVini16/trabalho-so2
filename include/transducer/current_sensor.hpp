#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"
#include <random>
#include <time>

/*
 * Classe específica para o transdutor de sensor de corrente, que sensoria a corrente do veículo.
 */
class CurrentSensor : public Transducer<SmartData::Unit::Current> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Current>::Type; // Tipo de dado numérico correspondente à unidade de corrente
    static Value sense() {
        static bool seeded = false;
        if (!seeded) {
            srand(time(nullptr) ^ (SmartData::Unit::Current & 0xFFFF));
            seeded = true;
        }
        // CurrentSensor - corrente entre 0 e 10A
        return (rand() % 100) / 10.0;
    }
};