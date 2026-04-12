#pragma once
#include <cstddef>

// classe base para as engines
class Engine {
    // os métodos são reescritos pelas filhas
    protected:
        // virtual significa que vai ser reescrito
        virtual int _send(const void* buf, size_t len) = 0;
        virtual void _handle(void* buf, size_t len) = 0;
        virtual ~Engine() = default;
};