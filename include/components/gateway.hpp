// gateway.hpp
#pragma once
#include "../nic/nic.hpp"
#include "../engine/raw_socket_engine.hpp"
#include "../engine/shm_engine.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"
#include "../observe/endpoint.hpp"
#include "../utils/ports.hpp"
#include "../smart_data/smart_data.hpp"
#include "../smart_data/interested_smart_data.hpp"
#include "../smart_data/responsive_smart_data.hpp"
#include "../transducer/velocimeter.hpp"
#include "../ethernet.hpp"

class Gateway {
public:
    using SDVector = std::vector<std::unique_ptr<SmartData>>;
    Gateway(const std::string& iface)
        : rs_nic(iface),
          protocol(&rs_nic),
          _communicator(&protocol,
                        Protocol::Address{rs_nic.address(), Ports::GATEWAY}),
          _endpoint(_communicator),        // observador bloqueante do communicator (receive())
          shm_nic(key(), rs_nic.address()) // chave fixa para comunicação local
    {
        protocol.attach_nic(&shm_nic); // protocol precisa conhecer a shm_nic para enviar mensagens locais
    }

    ~Gateway() { stop_smart_datas(); } // garante join das threads dos SmartData

    key_t key() const { return _make_key(rs_nic.address()); }

    bool send(const Message& msg) {
        return _communicator.send(&msg);
    }

    bool share(const Message& msg, Protocol::Port dst_port) {
        return _communicator.share(&msg, dst_port);
    }
    bool receive(Message& msg) {
        return _endpoint.receive(&msg);
    }

    Protocol::Address address() const {
        return _communicator.address();
    }

    void setup() {
        add_smart_datas(); // Método para adicionar SmartData específicos de cada componente
        start_smart_datas(); // Inicia o processamento dos SmartData associados a este componente
    }

    void add_smart_datas() {
        // Gateway interessado em velocidade: envia Interests periodicamente (período de 1ms)
        smart_data_units.push_back(
            std::make_unique<InterestedSmartData<SmartData::Unit::Velocity>>(
                &protocol, rs_nic.address(), 1000000ull));
        // Gateway responsivo em velocidade: responde Interests com leituras do Velocimeter
        smart_data_units.push_back(
            std::make_unique<ResponsiveSmartData<Velocimeter>>(
                &protocol, rs_nic.address()));
    }

    void start_smart_datas() {
        for (auto& sd : smart_data_units) {
            sd->start(); // Inicia o processamento de cada SmartData associado a este componente
        }
    }

    void stop_smart_datas() {
        for (auto& sd : smart_data_units) {
            sd->stop(); // sinaliza parada e faz join da thread de cada SmartData
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
    Endpoint             _endpoint; // observador bloqueante inscrito no _communicator
    NIC<ShmEngine>      shm_nic; // NIC para comunicação com componentes locais
    SDVector smart_data_units; // SmartData hospedados pelo Gateway
    Ethernet::Address _mac; // endereço MAC deste componente (pode ser útil para identificação e comunicação)
};