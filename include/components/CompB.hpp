#include "component.hpp"
#include "../smart_data/interested_smart_data.hpp"
#include "../smart_data/responsive_smart_data.hpp"
#include "../transducer/accelerometer.hpp"
#include "../transducer/thermometer.hpp"
#include "../smart_data/smart_data.hpp"
#include "../ethernet.hpp"
#include "../protocol.hpp"

/*
 * Classe de teste. Recebe aceleração, envia temperatura. Pode ser usada para testar a comunicação entre componentes, o funcionamento dos SmartData, etc.
*/

class CompB : public Component {
public:
    CompB(key_t key, Ethernet::Address mac) : Component(key, mac) {}
    void add_smart_datas() override {
        // Adiciona um InterestedSmartData para a unidade de temperatura, com um período de 1 segundo (1000000000 nanosegundos)
        smart_data_units.push_back(std::make_unique<InterestedSmartData<SmartData::Unit::Acceleration>>(&protocol, _mac, 1000000000ull));
        // Adiciona um ResponsiveSmartData para a unidade de aceleração
        smart_data_units.push_back(std::make_unique<ResponsiveSmartData<Thermometer>>(&protocol, _mac));
    }
};