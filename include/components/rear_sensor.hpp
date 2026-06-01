#pragma once
#include "component.hpp"
#include "../utils/type_code.hpp"

/*
 * Componente referente ao sensor traseiro do veículo.
 */
class RearSensor : public Component {
public:
    RearSensor(Protocol::Address address, key_t key, Ethernet::Address mac) : 
        Component(address, key, mac, 
            {
                {TypeCode::CURRENT_GEAR, 1000000}, // deseja receber a marcha atual a cada 1 segundo
                {TypeCode::VELOCITY, 1000000}, // deseja receber a velocidade a cada 1 segundo
            }, 
            {TypeCode::REAR_DISTANCE}) // produz a distância do sensor traseiro
    {}
};