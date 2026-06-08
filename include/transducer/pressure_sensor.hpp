#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"

/*
 * Classe específica para o transdutor de pressão, que sensoria a pressão do veículo.
 */
class PressureSensor : public Transducer<SmartData::Unit::Pressure> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Pressure>::Type; // Tipo de dado numérico correspondente à unidade de pressão
    static Value sense() {
        static bool seeded = false;
        if (!seeded) {
            srand(time(nullptr) ^ (SmartData::Unit::Pressure & 0xFFFF));
            seeded = true;
        }
        // PressureSensor - pressão entre 90kPa e 110kPa
        return 90000.0 + (rand() % 20000);
    }
};