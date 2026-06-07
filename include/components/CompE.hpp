#include "component.hpp"
#include "../smart_data/interested_smart_data.hpp"
#include "../smart_data/responsive_smart_data.hpp"
#include "../transducer/voltage_sensor.hpp"
#include "../smart_data/smart_data.hpp"
#include "../ethernet.hpp"
#include "../protocol.hpp"

/*
 * Classe de teste. Recebe corrente, envia voltagem. Pode ser usada para testar a comunicação entre componentes, o funcionamento dos SmartData, etc.
*/
class CompE : public Component {
public:
    CompE(key_t key, Ethernet::Address mac) : Component(key, mac) {}
    void add_smart_datas() override {
        // Adiciona um InterestedSmartData para a unidade de temperatura, com um período de 1 segundo (1000000000 nanosegundos)
        smart_data_units.push_back(std::make_unique<InterestedSmartData<SmartData::Unit::Current>>(&protocol, _mac, 1000000ull));
        // Adiciona um ResponsiveSmartData para a unidade de aceleração
        smart_data_units.push_back(std::make_unique<ResponsiveSmartData<VoltageSensor>>(&protocol, _mac));
    }
};