#pragma once
#include "component.hpp"
#include "../utils/type_code.hpp"

/*
 * Componente que representa o powertrain do veículo.
 */
class Powertrain : public Component {
public:
    Powertrain(Protocol::Address address, key_t key, Ethernet::Address mac) : 
        Component(address, key, mac,
            {
                {TypeCode::BRAKE_STATUS, 100000}, // deseja receber comandos de freio a cada 100ms
                {TypeCode::STEERING_ANGLE, 100000}, // deseja receber comandos de direção a cada 100ms
            },
            {TypeCode::VELOCITY, TypeCode::ACCELERATION, TypeCode::CURRENT_GEAR, TypeCode::ENGINE_RPM}) // produz velocidade, aceleração, marcha atual e RPM do motor

    {}
};