#pragma once
#include "component.hpp"

class Sensor : public Component {
public:
    Sensor(Protocol::Address address, key_t key, Ethernet::Address mac) : Component(address, key, mac) {}

    void run() {
        //std::cout << "[Sensor] Rodando com endereço " << communicator.address() << "\n";
        while(!g_stop) {
            Message msg;
            if(receive(msg)) {
                std::string txt(static_cast<char*>(msg.data()), msg.size());
                //std::cout << "[Sensor] recebeu: '" << txt << "'\n";

                // ecoa de volta para o gateway via share (broadcast local)
                Message resp;
                std::string reply = "sensor-ack: " + txt;
                std::memcpy(resp.data(), reply.c_str(), reply.size());
                resp.set_size(reply.size());
                share(resp, Ports::GATEWAY);
            }
        }
    }
};