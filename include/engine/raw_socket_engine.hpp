#pragma once
#include "engine.hpp"
#include "../ethernet.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>

/**
 * @brief An Engine for handling raw socket communication.
 *
 * A recepção é totalmente assíncrona e dirigida por sinal: o kernel gera
 * SIGIO sempre que há pacotes prontos no socket (O_ASYNC), e o handler apenas
 * acorda a thread de recepção via semáforo. Não há recvfrom bloqueante nem
 * polling por timeout — o recvfrom roda em modo não-bloqueante (O_NONBLOCK)
 * apenas para drenar a fila do kernel quando avisado.
 */
class RawSocketEngine : public Engine {
    public:
        RawSocketEngine(const std::string& iface) : _iface(iface), _running(true) {
            // 1. abre o socket
            _fd = socket(AF_PACKET, SOCK_RAW, htons(0x8888));
            if(_fd < 0)
                throw std::runtime_error("socket() falhou — rode como root");

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

            // 5. semáforo de notificação — sem_post é async-signal-safe, por isso
            //    usamos um sem_t POSIX (e não o wrapper C++20) dentro do handler.
            sem_init(&_notify_sem, 0, 0);

            // 6. registra a instância para o handler de sinal alcançá-la
            //    (limitação: um RawSocketEngine por processo, como no ShmEngine)
            _instance = this;

            // 7. instala o handler de SIGIO — disparado pelo kernel quando o
            //    socket fica legível
            struct sigaction sa{};
            sa.sa_handler = &RawSocketEngine::_signal_handler;
            sigemptyset(&sa.sa_mask);
            sigaddset(&sa.sa_mask, SIGIO);
            sa.sa_flags = 0;
            sigaction(SIGIO, &sa, nullptr);

            // 8. configura I/O assíncrono dirigido por sinal:
            //    - F_SETOWN: este processo recebe o SIGIO deste socket
            //    - O_ASYNC:  kernel gera SIGIO quando há dados para ler
            //    - O_NONBLOCK: recvfrom nunca bloqueia (retorna EAGAIN ao esvaziar)
            if(fcntl(_fd, F_SETOWN, getpid()) < 0)
                throw std::runtime_error("fcntl(F_SETOWN) falhou");
            int flags = fcntl(_fd, F_GETFL, 0);
            if(flags < 0 || fcntl(_fd, F_SETFL, flags | O_ASYNC | O_NONBLOCK) < 0)
                throw std::runtime_error("fcntl(O_ASYNC) falhou");
        }

        void start() {
            _thread = std::thread([this]{ _receive_loop(); });
        }

        void stop() {
            // idempotente: garante que a limpeza rode uma única vez
            if(!_running.exchange(false))
                return;

            // para de receber sinais deste socket antes de destruir o semáforo
            signal(SIGIO, SIG_IGN);

            sem_post(&_notify_sem); // acorda a thread para ela sair
            if(_thread.joinable())
                _thread.join();

            _instance = nullptr;
            close(_fd);
            sem_destroy(&_notify_sem);
        }

        ~RawSocketEngine() {
            stop();
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
            return Ethernet::Address();
        }

    protected:
        int _send(const void* buf, size_t len) override {

            // Set up the socket address structure for sending (broadcast)
            struct sockaddr_ll addr{};
            addr.sll_family  = AF_PACKET;
            addr.sll_protocol = htons(ETH_P_ALL);

            addr.sll_ifindex = _ifindex;
            addr.sll_halen   = 6;
            std::memset(addr.sll_addr, 0xff, 6);   // broadcast

            // Send the raw Ethernet frame (returns number of bytes sent or -1 on error)
            ssize_t sent =  sendto(_fd, buf, len, 0,
                        (struct sockaddr*)&addr, sizeof(addr));

            if(sent < 0)
                std::cerr << "[engine] sendto falhou: " << strerror(errno) << "\n";

            return static_cast<int>(sent);
        }

        // Thread de recepção: drena tudo que já está pendente e então dorme
        // no semáforo até o próximo SIGIO. O "drain-first" garante que nenhum
        // pacote seja perdido entre o bind e a instalação do handler, e cobre
        // o caso de vários pacotes chegarem para um único sinal (edge-trigger).
        void _receive_loop() {
            while(_running) {
                _drain();
                if(!_running)
                    break;
                // dorme até o kernel sinalizar chegada de novo pacote.
                // EINTR (outros sinais) apenas nos faz drenar de novo — inofensivo.
                sem_wait(&_notify_sem);
            }
        }

    private:
        // handler de sinal — só faz sem_post (async-signal-safe), nada de I/O
        static void _signal_handler(int) {
            if(_instance)
                sem_post(&_instance->_notify_sem);
        }

        // lê todos os frames pendentes no socket sem bloquear
        void _drain() {
            uint8_t buf[sizeof(Ethernet::Frame)];
            while(_running) {
                ssize_t len = recvfrom(_fd, buf, sizeof(buf), 0, nullptr, nullptr);
                if(len < 0) {
                    if(errno == EWOULDBLOCK || errno == EAGAIN)
                        break;      // fila do kernel esvaziada
                    if(errno == EINTR)
                        continue;   // interrompido por sinal, tenta de novo
                    break;          // erro real (ex.: fd fechado no stop())
                }
                if(len == 0)
                    break;
                auto* frame = reinterpret_cast<Ethernet::Frame*>(buf);
                // ignora frames que não são do projeto
                if(ntohs(frame->type) == 0x8888)
                    _handle(buf, static_cast<size_t>(len));
            }
        }

        sem_t _notify_sem;          // acordado pelo SIGIO; drena a thread de recepção
        int _fd;                    // File descriptor for the raw socket
        std::string _iface;         // Network interface to bind to (e.g., "eth0")
        std::thread _thread;        // Thread for listening to incoming data
        std::atomic<bool> _running; // Flag to control the listening thread
        Ethernet::Address _address; // MAC address of the interface
        int _ifindex;               // Interface index for sending packets

        // ponteiro para a instância alcançada pelo handler de sinal
        inline static RawSocketEngine* _instance = nullptr;
};
