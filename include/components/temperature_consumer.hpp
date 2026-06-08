#include "component.hpp"
#include "../smart_data/interested_smart_data.hpp"
#include "../smart_data/responsive_smart_data.hpp"
#include "../transducer/velocimeter.hpp"
#include "../smart_data/smart_data.hpp"
#include "../ethernet.hpp"
#include "../protocol.hpp"

class TemperatureConsumer : public Component {
public:
    TemperatureConsumer(key_t key, Ethernet::Address mac) : Component(key, mac) {}
    void add_smart_datas() override {
        // Adiciona um ResponsiveSmartData para a unidade de aceleração
        smart_data_units.push_back(std::make_unique<InterestedSmartData<SmartData::Unit::Temperature>>(&protocol, _mac, 1000000ull));    
    }
};