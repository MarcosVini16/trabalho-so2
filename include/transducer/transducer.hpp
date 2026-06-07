#pragma once
#include "../smart_data/smart_data.hpp"

/*
 * Classe base para os transdutores, que são responsáveis por sensoriar os dados do ambiente e fornecer esses dados para os SmartData associados a eles.
 */
template<SmartData::Unit::Code UNIT_CODE>
class Transducer {
public:
    static constexpr SmartData::Unit::Code UNIT = UNIT_CODE; // Código da unidade   
    using Value = typename SmartData::Unit::Get<UNIT_CODE>::Type; // Tipo de dado numérico correspondente à unidade do transdutor

    static Value sense(); // Método virtual puro para sensoriar os dados do ambiente, a ser implementado pelas subclasses de Transducer
    virtual ~Transducer() = default; // Destrutor virtual padrão
};