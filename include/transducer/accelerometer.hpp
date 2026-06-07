#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"

/*
 * Classe específica para o transdutor de acelerômetro, que sensoria a aceleração do veículo.
 */
class Accelerometer : public Transducer<SmartData::Unit::Acceleration> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Acceleration>::Type; // Tipo de dado numérico correspondente à unidade de aceleração
    Value static sense() {
        return 9.81f; // Retorna uma aceleração fixa de 9.81 m/s2 para fins de teste (aceleração devido à gravidade)
    }
};