// gateway.hpp
#pragma once
#include "../nic/nic.hpp"
#include "../engine/raw_socket_engine.hpp"
#include "../engine/shm_engine.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"
#include "../utils/ports.hpp"

class Gateway {
public:
    Gateway(const std::string& iface)
        : rs_nic(iface),
          protocol(&rs_nic),
          _communicator(&protocol,
                        Protocol::Address{rs_nic.address(), Ports::GATEWAY})
    {
        
    }

    ~Gateway() = default;

    bool send(const Message& msg) {
        return _communicator.send(&msg);
    }

    bool receive(Message& msg) {
        return _communicator.receive(&msg);
    }

    Protocol::Address address() const {
        return _communicator.address();
    }

private:
    NIC<RawSocketEngine> rs_nic; // NIC para comunicação com a rede externa (outros veículos)
    Protocol protocol; // Protocolo de comunicação
    Communicator         _communicator; // Camada de comunicação para enviar/receber mensagem
    // NIC<ShmEngine>      shm_nic; // NIC para comunicação com componentes locais
};