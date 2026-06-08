#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"

/*
 * Classe específica para o transdutor de voltagem, que sensoria a voltagem do veículo.
 */
class VoltageSensor : public Transducer<SmartData::Unit::Voltage> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Voltage>::Type; // Tipo de dado numérico correspondente à unidade de voltagem
    static Value sense() {
        static bool seeded = false;
        if (!seeded) {
            srand(time(nullptr) ^ (SmartData::Unit::Voltage & 0xFFFF));
            seeded = true;
        }
        // VoltageSensor - voltagem entre 11V e 14V
        return 11.0 + (rand() % 30) / 10.0;
    }
};
