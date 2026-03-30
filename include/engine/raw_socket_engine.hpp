#pragma once
#include "engine.hpp"
#include "../ethernet.hpp"
#include <string>
#include <thread>
#include <atomic>

/**
 * @brief An Engine for handling raw socket communication
 */
class RawSocketEngine : public Engine {
    public:
        RawSocketEngine(const std::string& iface);
        ~RawSocketEngine();

        Ethernet::Address read_address();  // lê o MAC da interface
        Ethernet::Address address() const { return _address; }
        

    protected:
        /*
         * @brief Sends data through the raw socket
         * @param buf Pointer to the buffer containing the data to send
         * @param len Length of the data to send
         * @return Number of bytes sent, or -1 on error
         */
        int  _send(const void* buf, size_t len) override;
        /*
         * @brief Handles incoming data from the raw socket
         * @param buf Pointer to the buffer containing the received data
         * @param len Length of the received data
         */
        void _handle(void* buf, size_t len) override;

        /*
         * @brief Internal loop for receiving data from the raw socket
         * This function runs in a separate thread and continuously listens for incoming packets.
         * When a packet is received, it calls the _handle() method to process it.
         */
        void _receive_loop();

        

    private:
        int _fd; // File descriptor for the raw socket
        std::string _iface; // Network interface to bind to (e.g., "eth0", "wlan0", etc.)
        std::thread _thread; // Thread for listening to incoming data
        std::atomic<bool> _running; // Flag to control the listening thread
        Ethernet::Address _address; // MAC address of the interface
        int _ifindex; // Interface index for sending packets
};