#pragma once
#include "../nic/nic_base.hpp"
#include "../nic/nic.hpp"
#include "../engine/shm_engine.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

/*
 * Classe base para os componentes do veículo. 
 * Pode se comunicar com outros componentes locais via memória compartilhada.
 */
class Component {
public:

    Component(Protocol::Address address, key_t key, Ethernet::Address mac)
        : nic(key, mac), protocol(&nic), communicator(&protocol, address) {};

    ~Component() = default;

    /*
     * @brief Envia uma mensagem para um destino específico.
     * @param msg A mensagem a ser enviada.
     * @return true se a mensagem foi enviada com sucesso, false caso contrário.
    */
    bool send(const Message& msg) {
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
};