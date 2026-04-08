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
    static Ethernet::Address read_address() {
        return Ethernet::Address{};
    }
protected:
    int  _send(const void* buf, size_t len) override;
    void _handle(void* buf, size_t len) override;
private:
    // Additional members for managing shared memory (e.g., file descriptor, memory pointer, etc.) would be defined here.
};