#pragma once
#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"

/*
 * Classe específica para o transdutor de velocímetro, que sensoria a velocidade do veículo.
 */
class Velocimeter : public Transducer<SmartData::Unit::Velocity> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Velocity>::Type; // Tipo de dado numérico correspondente à unidade de velocidade
    Value static sense() {
        return 15.0f; // Retorna uma velocidade fixa de 15 m/s para fins de teste
    }
};