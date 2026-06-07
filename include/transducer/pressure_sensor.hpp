#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"

/*
 * Classe específica para o transdutor de pressão, que sensoria a pressão do veículo.
 */
class PressureSensor : public Transducer<SmartData::Unit::Pressure> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Pressure>::Type; // Tipo de dado numérico correspondente à unidade de pressão
    Value static sense() {
        return 200.0f; // Retorna uma pressão fixa de 200 kPa para fins de teste
    }
};