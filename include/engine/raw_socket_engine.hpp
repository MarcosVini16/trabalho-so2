#pragma once
#include "engine.hpp"
#include <string>
#include <thread>
#include <atomic>

/**
 * @brief An Engine for handling raw socket communication
 */
class RawSocketEngine : public Engine {
    public:
        Raw_Socket_Engine(const std::string& iface);
        ~Raw_Socket_Engine();

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

    private:
        int _fd; // File descriptor for the raw socket
        std::string _interface; // Network interface to bind to (e.g., "eth0", "wlan0", etc.)
        std::thread _thread; // Thread for listening to incoming data
        std::atomic<bool> _running; // Flag to control the listening thread
};