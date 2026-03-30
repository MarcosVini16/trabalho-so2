#pragma once
#include "component.hpp"

class Sensor : public Component {
public:
    Sensor(Protocol::Address address) : Component(address) {}
    ~Sensor() override = default;
};