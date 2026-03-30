#pragma once
#include "engine.hpp"
#include "../ethernet.hpp"

/*
 * A shared memory engine for inter-process communication.
 * This engine allows multiple processes to communicate by sharing a memory region.
 * It uses a simple protocol to send and receive Ethernet frames through the shared memory.
 */
class ShmEngine : public Engine {
public:
    ShmEngine();
    ~ShmEngine();
    static Ethernet::Address read_address() {
        return Ethernet::Address{{0,0,0,0,0,1}};
    }
protected:
    int  _send(const void* buf, size_t len) override;
    void _handle(void* buf, size_t len) override;
private:
    // Additional members for managing shared memory (e.g., file descriptor, memory pointer, etc.) would be defined here.
};