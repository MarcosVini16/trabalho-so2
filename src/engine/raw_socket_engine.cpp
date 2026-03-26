// engine_raw.cpp
#include "../../include/engine/raw_socket_engine.hpp"
#include "../../include/ethernet.hpp"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>


RawSocketEngine::RawSocketEngine(const std::string& iface)
    : _iface(iface), _running(true)
{
    // Open a raw socket, requires root privileges
    _fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    //
    if(_fd < 0)
        throw std::runtime_error("socket() falhou — rode como root");

    // Bind the socket to the specified interface (e.g., "eth0")
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);
    // If the interface index cannot be obtained, throw an error
    if(ioctl(_fd, SIOCGIFINDEX, &ifr) < 0)
        throw std::runtime_error("interface não encontrada: " + iface);

    
    // Set up the socket address structure for binding (ll = link layer)
    struct sockaddr_ll addr{};
    addr.sll_family   = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex  = ifr.ifr_ifindex;

    // Bind the socket to the interface (throws on failure)
    if(bind(_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() falhou");

    // Start the receiving thread (which will call _handle() for each received packet)
    _thread = std::thread([this]{ _receive_loop(); });
}

RawSocketEngine::~RawSocketEngine() {
    _running = false;
    close(_fd);         // 
    if(_thread.joinable())
        _thread.join();
}

int RawSocketEngine::_send(const void* buf, size_t len) {
    // Set up the socket address structure for sending (broadcast)
    struct sockaddr_ll addr{};
    addr.sll_family  = AF_PACKET;
    addr.sll_ifindex = 0; 
    addr.sll_halen   = 6;
    std::memset(addr.sll_addr, 0xff, 6);   // broadcast

    // Send the raw Ethernet frame (returns number of bytes sent or -1 on error)
    return sendto(_fd, buf, len, 0,
                  (struct sockaddr*)&addr, sizeof(addr));
}

void RawSocketEngine::_receive_loop() {
    // Buffer to hold incoming Ethernet frames (max size is sizeof(Ethernet::Frame))
    uint8_t buf[sizeof(Ethernet::Frame)];
    while(_running) {
        // Receive data from the raw socket (blocks until a packet is received)
        ssize_t len = recvfrom(_fd, buf, sizeof(buf), 0, nullptr, nullptr);
        // If data was received, call the _handle() method to process it
        if(len > 0)
            _handle(buf, static_cast<size_t>(len));
    }
}

void RawSocketEngine::_handle(void* buf, size_t len) {
    // For demonstration, we just print the length of the received packet
    std::cout << "Received packet of length: " << len << " bytes" << std::endl;
    // In a real implementation, you would parse the Ethernet frame and handle it accordingly
}

int main() {
    try {
        RawSocketEngine engine("enp0s1"); // Substitua "enp0s1" pelo nome da sua interface de rede
        // O engine agora está rodando e processando pacotes
        std::this_thread::sleep_for(std::chrono::seconds(10)); // exemplo de tempo de execução
    } catch(const std::exception& ex) {
        std::cerr << "Erro: " << ex.what() << std::endl;
        return 1;
    }
    return 0;
}