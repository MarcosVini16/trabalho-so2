#include "../smart_data/smart_data.hpp"
#include "transducer.hpp"

/*
 * Classe específica para o transdutor de termômetro, que sensoria a temperatura ambiente.
 */
class Thermometer : public Transducer<SmartData::Unit::Temperature> {
public:
    using Value = typename SmartData::Unit::Get<SmartData::Unit::Temperature>::Type; // Tipo de dado numérico correspondente à unidade de temperatura
    Value sense() override {
        return 298.15f; // Retorna uma temperatura fixa de 25 graus Celsius (298.15 Kelvin) para fins de teste
    }
};