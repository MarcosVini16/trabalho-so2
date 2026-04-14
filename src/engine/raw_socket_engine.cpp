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
#include <cerrno>
#include <signal.h>
#include <fcntl.h>

// ponteiro global para a instância 
static RawSocketEngine* _instance = nullptr;

// agrupa multiplos frames em um único SIGIO
// esse handler é chamado uma vez, não uma vez por frame
static void sigio_handler(int) {
    // acorda a thread de recepção
    if(_instance) _instance->_notify();
}


RawSocketEngine::RawSocketEngine(const std::string& iface)
    : _iface(iface), _running(true)
{
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

    // 5. configura SIGIO

    // salva o ponteiro pra essa instância em uma variável global
    // handler de sinal não pode ter método de instância
    _instance = this;

    // cria a estrutura que define o comportamento do sinal
    struct sigaction sa{};

    // define qual função será chamada quando SIGIO chegar
    sa.sa_handler = sigio_handler;

    // garante que nenhum sinal está bloqueando durante o handler
    sigemptyset(&sa.sa_mask);

    // sem flags especiais
    sa.sa_flags = 0;

    // registra o handler para o sinal SIGIO no SO
    sigaction(SIGIO, &sa, nullptr);

    // diz ao SO que esse processo é dono do socket
    // ou seja, é ele que vai receber o SIGIO quando dados chegarem
    fcntl(_fd, F_SETOWN, getpid());

    // ativa o modo assíncrono no socket
    // a partir daqui o SO dispara SIGIO automaticamente quando dados chegarem
    fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL) | O_ASYNC);

    // 6. inicia thread de recepção
    _thread = std::thread([this]{ _receive_loop(); });
}

RawSocketEngine::~RawSocketEngine() {
    _running = false;
    close(_fd);         // 
    if(_thread.joinable())
        _thread.join();
}

int RawSocketEngine::_send(const void* buf, size_t len) {
    auto* frame = reinterpret_cast<const Ethernet::Frame*>(buf);
    std::cout << "[engine] enviando frame EtherType=0x" 
              << std::hex << ntohs(frame->type) 
              << " len=" << std::dec << len << "\n";
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

    
    std::cout << "[engine] sendto retornou=" << sent 
              << " ifindex=" << _ifindex << "\n";
    
    return static_cast<int>(sent);
}

void RawSocketEngine::_receive_loop() {
    uint8_t buf[sizeof(Ethernet::Frame)];


    while(_running) {

        // thread dorme aqui esperando o SIGIO chamar _notify() -> _sem.v()
        _sem.p();

        // loop de dreno -> lê tudo que tá no socket
        while(_running) {

            // MSG_DONTWAIT faz retornar imediatamente se não houver mais dados
            ssize_t len = recvfrom(_fd, buf, sizeof(buf), MSG_DONTWAIT, nullptr, nullptr);

            // len <= 0 significa que o socket está vazio — para o dreno
            if(len <= 0) break;

            // interpreta o buffer como um frame Ethernet
            auto* frame = reinterpret_cast<Ethernet::Frame*>(buf);

            // filtra só frames do projeto (EtherType 0x8888)
            if(ntohs(frame->type) == 0x8888)
                // passa o frame para a NIC processar via Observer
                _handle(buf, static_cast<size_t>(len));
        }
        // socket drenado — volta para _sem.p() e dorme até o próximo SIGIO
    }
}


Ethernet::Address RawSocketEngine::read_address() {
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, _iface.c_str(), IFNAMSIZ - 1);
    if(ioctl(_fd, SIOCGIFHWADDR, &ifr) < 0)
        throw std::runtime_error("falha ao ler MAC de " + _iface);

    Ethernet::Address addr{};
    std::memcpy(addr.bytes, ifr.ifr_hwaddr.sa_data, 6);
    return addr;
}