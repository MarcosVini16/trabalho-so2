#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"
/*
 * Classe específica para o transdutor de sensor de corrente, que sensoria a corrente do veículo.
 */
class CurrentSensor : public Transducer<SmartData::Unit::Current> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Current>::Type; // Tipo de dado numérico correspondente à unidade de corrente
    Value static sense() {
        return 5.0f; // Retorna uma corrente fixa de 5 A para fins de teste
    }
};