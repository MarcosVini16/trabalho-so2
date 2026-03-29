// engine.hpp
#pragma once
#include <cstddef>

/*
 * Base class for all engine types
 * Defines the interface for sending and handling frames.
 */
class Engine {
    protected:
        virtual int  _send(const void* buf, size_t len) = 0;
        virtual void _handle(void* buf, size_t len) = 0;
        virtual ~Engine() = default;
};