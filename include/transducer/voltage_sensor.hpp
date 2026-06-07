#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"

/*
 * Classe específica para o transdutor de voltagem, que sensoria a voltagem do veículo.
 */
class VoltageSensor : public Transducer<SmartData::Unit::Voltage> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Voltage>::Type; // Tipo de dado numérico correspondente à unidade de voltagem
    Value static sense() {
        return 12.0f; // Retorna uma voltagem fixa de 12 V para fins de teste
    }
};
