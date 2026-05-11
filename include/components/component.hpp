#pragma once
#include "../nic/nic_base.hpp"
#include "../nic/nic.hpp"
#include "../engine/shm_engine.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"
#include <time.h>
#include <cstdint>

extern volatile sig_atomic_t g_stop; // variável global para sinalizar parada total, definida em vehicle_main.cpp

/*
 * Classe base para os componentes do veículo. 
 * Pode se comunicar com outros componentes locais via memória compartilhada.
 */
class Component {
public:

    Component(Protocol::Address address, key_t key, Ethernet::Address mac)
        : nic(key, mac), protocol(&nic), communicator(&protocol, address) {
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
    key_t clock_key; // chave para o segmento de memória compartilhada do relógio (pode ser útil para componentes que precisam acessar o relógio sincronizado)
};