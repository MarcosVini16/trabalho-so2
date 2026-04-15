#pragma once
#include "component.hpp"

class Actuator : public Component {
public:
    Actuator(Protocol::Address address, key_t key, Ethernet::Address mac) : Component(address, key, mac) {}
};