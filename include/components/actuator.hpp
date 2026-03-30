#pragma once
#include "component.hpp"

class Actuator : public Component {
public:
    Actuator(Protocol::Address address) : Component(address) {}
    ~Actuator() override = default;
};