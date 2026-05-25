#pragma once
#include "../nic/nic_base.hpp"
#include "../nic/nic.hpp"
#include "../engine/shm_engine.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"
#include "../utils/position.hpp"
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
    // envia uma message já montada
    bool send(const Message& msg) {
        Message m = msg;
        m.set_origin(communicator.address().paddr, Position::quadrant());
        return communicator.send(&m);
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
        // cria uma mensagem vazia, copia os dados pro buffer e registra o tamanho
        Message msg;
        std::memcpy(msg.data(), data, size);
        msg.set_size(size);

        // captura o timestamp e converte para um único número em nanosegundos
        timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);
        uint64_t ts = current_time.tv_sec * 1000000000ULL + current_time.tv_nsec;

        // preenche o origin da mensagem com o componente e o quadrante. preenche o timestamp
        msg.set_origin(communicator.address().paddr, Position::quadrant());
        msg.set_timestamp(ts);
        
        // envia a mensagem pelo comunicator
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