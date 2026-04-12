#pragma once
#include "engine.hpp"
#include "../ethernet.hpp"
#include <string>
#include <thread>
#include <atomic>

class RawSocketEngine : public Engine {
    public:
        RawSocketEngine(const std::string& iface);
        ~RawSocketEngine();

        Ethernet::Address read_address();  // lê o MAC da interface
        Ethernet::Address address() const { return _address; }
        

    protected:

        int  _send(const void* buf, size_t len) override;
        /*
         * @brief Handles incoming data from the raw socket
         * @param buf Pointer to the buffer containing the received data
         * @param len Length of the received data
         */
        //void _handle(void* buf, size_t len) override;

        /*
         * @brief Internal loop for receiving data from the raw socket
         * This function runs in a separate thread and continuously listens for incoming packets.
         * When a packet is received, it calls the _handle() method to process it.
         */
        void _receive_loop();

        

    private:
        // file descriptor do socket (identifica o socket)
        int _fd;
        // nome da interface de rede
        std::string _iface;
        // uma thread do sistema operacional
        std::thread _thread;
        // flag atômica pra thread saber quando parar
        std::atomic<bool> _running;
        // MAC da interface
        Ethernet::Address _address;
        // índice numérico da interface
        int _ifindex;
};