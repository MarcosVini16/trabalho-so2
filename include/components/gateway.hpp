// gateway.hpp
#pragma once
#include "../nic/nic.hpp"
#include "../engine/raw_socket_engine.hpp"
#include "../engine/shm_engine.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"
#include "../utils/ports.hpp"
#include "../smart_data/smart_data.hpp"
#include "../ethernet.hpp"
class Gateway {
public:
    using SDVector = std::vector<std::unique_ptr<SmartData>>;
    Gateway(const std::string& iface)
        : rs_nic(iface),
          protocol(&rs_nic),
          _communicator(&protocol,
                        Protocol::Address{rs_nic.address(), Ports::GATEWAY}),
        shm_nic(key(), rs_nic.address()) // chave fixa para comunicação local
    {
        protocol.attach_nic(&shm_nic); // protocol precisa conhecer a shm_nic para enviar mensagens locais
    }

    ~Gateway() = default;

    key_t key() const { return _make_key(rs_nic.address()); }

    bool send(const Message& msg) {
        return _communicator.send(&msg);
    }

    bool share(const Message& msg, Protocol::Port dst_port) {
        return _communicator.share(&msg, dst_port);
        
    }
    bool receive(Message& msg) {
        return _communicator.receive(&msg);
    }

    Protocol::Address address() const {
        return _communicator.address();
    }

    void setup() {
        add_smart_datas(); // Método para adicionar SmartData específicos de cada componente
        start_smart_datas(); // Inicia o processamento dos SmartData associados a este componente
    }

    void add_smart_datas() {}

    void start_smart_datas() {
        for (auto& sd : smart_data_units) {
            sd->start(); // Inicia o processamento de cada SmartData associado a este componente
        }
    }

private:
    static key_t _make_key(Ethernet::Address mac) {
        key_t k = 0;
        std::memcpy(&k, mac.bytes + 2, 4);
        return k ? k : 1;
    }
    NIC<RawSocketEngine> rs_nic; // NIC para comunicação com a rede externa (outros veículos)
    Protocol protocol; // Protocolo de comunicação
    Communicator         _communicator; // Camada de comunicação para enviar/receber mensagem
    NIC<ShmEngine>      shm_nic; // NIC para comunicação com componentes locais
    SDVector smart_data_units; // lista de SmartData associados a este componente (pode ser útil para gerenciar o ciclo de vida dos SmartData)
    Ethernet::Address _mac; // endereço MAC deste componente (pode ser útil para identificação e comunicação)
};