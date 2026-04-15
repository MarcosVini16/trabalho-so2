// engine.hpp
#pragma once
#include <cstddef>

/*
 * Classe base para uma Engine de comunicação entre processos. 
 * Cada implementação específica deve implementar os métodos de envio e manipulação de mensagens.
 */
class Engine {
    protected:
        virtual int  _send(const void* buf, size_t len) = 0;
        virtual void _handle(void* buf, size_t len) = 0;
        virtual ~Engine() = default;
};