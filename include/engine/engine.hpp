// engine.hpp
#pragma once
#include <cstddef>
#include "../ethernet.hpp"

/*
 * Classe base para uma Engine de comunicação entre processos. 
 * Cada implementação específica deve implementar os métodos de envio e manipulação de mensagens.
 */
class Engine {
    protected:
        virtual int  _send(const void* buf, size_t len) = 0;
        virtual void _handle(void* buf, size_t len) = 0;
        virtual ~Engine() = default;
        /*
         * Retorna o endereço para qual essa Engine espera enviar mensagens. 
         * Por exemplo, para o RawSocketEngine, isso pode ser o endereço de broadcast, 
         * enquanto para o ShmEngine, isso pode ser o próprio endereço da instância.
         */
        virtual Ethernet::Address expected_dst() const = 0;

};