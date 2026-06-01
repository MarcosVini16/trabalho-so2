#pragma once
#include "../nic/nic_base.hpp"
#include "../nic/nic.hpp"
#include "../engine/shm_engine.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"
#include "../utils/type_code.hpp"
#include <time.h>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

extern volatile sig_atomic_t g_stop; // variável global para sinalizar parada total, definida em vehicle_main.cpp

/*
 * Classe base para os componentes do veículo. 
 * Pode se comunicar com outros componentes locais via memória compartilhada.
 */
class Component {
public:

    using TypeMap = std::unordered_map<TypeCode, uint64_t>;
    using TypeSet = std::unordered_set<TypeCode>;

    Component(Protocol::Address address, key_t key, Ethernet::Address mac, TypeMap interest_periods = {}, TypeSet produced_types = {})
        : nic(key, mac), protocol(&nic), communicator(&protocol, address), interest_periods(interest_periods), produced_types(produced_types) {
    };

    ~Component() = default;

    /*
     * @brief Envia uma mensagem para um destino específico.
     * @param msg A mensagem a ser enviada.
     * @return true se a mensagem foi enviada com sucesso, false caso contrário.
    */
    bool send(const Message& msg) {
        return communicator.send(&msg);
    }

    bool share(const Message& msg, Protocol::Port dst_port) {
        return communicator.share(&msg, dst_port);
    }

    /*
     * @brief Constrói e envia uma mensagem a partir de dados brutos.
     * @param data Ponteiro para os dados a serem enviados.
     * @param size Tamanho dos dados em bytes.
     * @return true se os dados foram enviados com sucesso, false caso contrário.
    */
    bool send(const void* data, unsigned int size) {
        Message msg;
        std::memcpy(msg.data(), data, size);
        msg.set_size(size);
        msg.set_src(communicator.address().paddr); // Define o endereço de origem da mensagem
        timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);
        uint64_t ts = current_time.tv_sec * 1000000000ULL + current_time.tv_nsec; // Convert to nanoseconds
        msg.set_timestamp(ts);
        return communicator.send(&msg);
    }
    /*
     * @brief Recebe uma mensagem, bloqueando até que uma mensagem esteja disponível.
     * @param msg Referência para um objeto Message onde a mensagem recebida será armazenada.
     * @return true se a mensagem foi recebida com sucesso, false caso contrário.
    */
    bool receive(Message& msg) {
        return communicator.receive(&msg);
    }

    /*
     * @brief Retorna o endereço (MAC address) do componente.
     * @return O endereço do componente.
    */
    Protocol::Address address() const {
        return communicator.address();
    }

protected:
    NIC<ShmEngine> nic; // NIC para comunicação com outros componentes locais.
    Protocol protocol; // Protocolo de comunicação
    Communicator communicator; // Camada de comunicação (mais alto nível) para enviar/receber mensagens
    TypeMap interest_periods; // período que componente deseja receber cada tipo de dado (em microsegundos)
    TypeMap response_periods = {}; // período que deve enviar cada dado - preenchido conforme interesses recebidos
    TypeSet produced_types; // tipos de dados que este componente produz
};