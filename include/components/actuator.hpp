#pragma once
#include "component.hpp"

class Actuator : public Component {
public:
    Actuator(Protocol::Address address, key_t key, Ethernet::Address mac) : Component(address, key, mac) {}

    void run() {
        std::cout << "[Actuator] Rodando com endereço " << communicator.address() << "\n";
        while(true) {
            Message msg;
            if(receive(msg)) {
                std::string txt(static_cast<char*>(msg.data()), msg.size());
                std::cout << "[Actuator] recebeu: '" << txt << "'\n";
            }
        }
    }
};