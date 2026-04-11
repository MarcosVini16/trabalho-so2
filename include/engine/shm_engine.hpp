#pragma once
#include "engine.hpp"
#include "../ethernet.hpp"

/*
 * Uma Engine para comunicação entre processos usando memória compartilhada.
 * Cada componente do veículo (sensor, atuador, etc.) pode usar essa engine para enviar e receber mensagens de outros componentes do mesmo veículo.
 */
class ShmEngine : public Engine {
public:
    ShmEngine();
    ~ShmEngine();
    // Função para descobrir endereço MAC
    void set_address(const Ethernet::Address& addr) { _address = addr; }
    Ethernet::Address address() const { return _address; }
    
protected:
    int  _send(const void* buf, size_t len) override;
    void _handle(void* buf, size_t len) override;
private:
    Ethernet::Address _address; // Endereço MAC do componente, usado para identificação
    // Additional members for managing shared memory (e.g., file descriptor, memory pointer, etc.) would be defined here.
};