#pragma once
#include "../utils/semaphore.hpp"
#include "engine.hpp"
#include "../ethernet.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <iostream>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>

/**
 * @brief An Engine for handling raw socket communication
 */
class RawSocketEngine : public Engine {
    public:
        RawSocketEngine(const std::string& iface) : _iface(iface), _running(true) {
            // 1. abre o socket
            _fd = socket(AF_PACKET, SOCK_RAW, htons(0x8888));
            if(_fd < 0)
                throw std::runtime_error("socket() falhou — rode como root");
            
            timeval tv{};
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms
            setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)); // timeout para recv

            // 2. busca o índice — preenche _ifindex
            struct ifreq ifr{};
            std::strncpy(ifr.ifr_name, iface.c_str(), IFNAMSIZ - 1);
            if(ioctl(_fd, SIOCGIFINDEX, &ifr) < 0)
                throw std::runtime_error("interface não encontrada: " + iface);
            _ifindex = ifr.ifr_ifindex;  // salva aqui

            // 3. lê o MAC — reusa o mesmo ifr
            if(ioctl(_fd, SIOCGIFHWADDR, &ifr) < 0)
                throw std::runtime_error("falha ao ler MAC de " + iface);
            std::memcpy(_address.bytes, ifr.ifr_hwaddr.sa_data, 6);

            // 4. bind usando _ifindex já preenchido
            struct sockaddr_ll addr{};
            addr.sll_family   = AF_PACKET;
            addr.sll_protocol = htons(0x8888);
            addr.sll_ifindex  = _ifindex;
            if(bind(_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
                throw std::runtime_error("bind() falhou");

            // 5. inicia thread de recepção
            _thread = std::thread([this]{ _receive_loop(); });
        }

        ~RawSocketEngine() {
            _running = false;
            close(_fd);         // fecha o socket para interromper o recv
            if(_thread.joinable())
                _thread.join();
            std::cout << "[raw] processo " << getpid() << " saiu\n";
        }

        Ethernet::Address read_address() {
            struct ifreq ifr{};
            std::strncpy(ifr.ifr_name, _iface.c_str(), IFNAMSIZ - 1);
            if(ioctl(_fd, SIOCGIFHWADDR, &ifr) < 0)
                throw std::runtime_error("falha ao ler MAC de " + _iface);

            Ethernet::Address addr{};
            std::memcpy(addr.bytes, ifr.ifr_hwaddr.sa_data, 6);
            return addr;
        }

        Ethernet::Address address() const { return _address; }

        Ethernet::Address expected_dst() const override {
            // para comunicação local, esperamos broadcast
            return Ethernet::Address::BROADCAST();
        }
    
    protected:
        int _send(const void* buf, size_t len) override {
            auto* frame = reinterpret_cast<const Ethernet::Frame*>(buf);
            //std::cout << "[engine] enviando frame EtherType=0x" 
            //        << std::hex << ntohs(frame->type) 
            //        << " len=" << std::dec << len << "\n";
            // Set up the socket address structure for sending (broadcast)
            struct sockaddr_ll addr{};
            addr.sll_family  = AF_PACKET;
            addr.sll_protocol = htons(ETH_P_ALL);
            // Why 0 and not _ifindex? Because we want to send to the broadcast address, so we set the interface index to 0 and specify the destination MAC address as broadcast.
            addr.sll_ifindex = _ifindex; 
            addr.sll_halen   = 6;
            std::memset(addr.sll_addr, 0xff, 6);   // broadcast

            // Send the raw Ethernet frame (returns number of bytes sent or -1 on error)
            ssize_t sent =  sendto(_fd, buf, len, 0,
                        (struct sockaddr*)&addr, sizeof(addr));
            
            if(sent < 0)
                std::cerr << "[engine] sendto falhou: " << strerror(errno) << "\n";

            
            //std::cout << "[engine] sendto retornou=" << sent 
            //        << " ifindex=" << _ifindex << "\n";
            
            return static_cast<int>(sent);
        }

        void _receive_loop() {
            uint8_t buf[sizeof(Ethernet::Frame)];
            while(_running) {
                ssize_t len = recvfrom(_fd, buf, sizeof(buf), 0, nullptr, nullptr);
                if (len < 0) {
                    if(errno == EWOULDBLOCK || errno == EAGAIN) {
                        // timeout, apenas continua esperando
                        continue;
                    } else {
                        break;
                    }
                }
                if(len > 0) {
                    auto* frame = reinterpret_cast<Ethernet::Frame*>(buf);
                    uint16_t etype = ntohs(frame->type);
                    // ignora frames que não são do projeto
                    if(etype == 0x8888)
                        _handle(buf, static_cast<size_t>(len));
                }
            }
        }
        

    private:
        // semáforo que sincroniza o SIGIO com a thread de recepção
        Semaphore _sem{0};
        int _fd; // File descriptor for the raw socket
        std::string _iface; // Network interface to bind to (e.g., "eth0", "wlan0", etc.)
        std::thread _thread; // Thread for listening to incoming data
        std::atomic<bool> _running; // Flag to control the listening thread
        Ethernet::Address _address; // MAC address of the interface
        int _ifindex; // Interface index for sending packets
};