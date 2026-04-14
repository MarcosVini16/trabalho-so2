#pragma once
#include "component.hpp"

class Sensor : public Component {
public:
    Sensor(Protocol::Address address, key_t key, Ethernet::Address mac) : Component(address, key, mac) {}
};