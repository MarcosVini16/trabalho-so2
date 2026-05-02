// app/vehicle_main.cpp
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>
#include <string>
#include "../include/vehicle.hpp"
#include "../include/utils/ports.hpp"

// ---------------------------------------------------------------------------
// Setup do signal de parada total
// ---------------------------------------------------------------------------

static volatile sig_atomic_t g_stop = 0;

extern "C" void on_stop(int) {
    g_stop = 1;
}

void setup_signals() {
    struct sigaction sa{};
    sa.sa_handler = on_stop;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGALRM, &sa, nullptr);
}

// ---------------------------------------------------------------------------
// Funções dos processos filhos (componentes intra-VM)
// Cada um constrói seus próprios objetos APÓS o fork — nunca antes.
// ---------------------------------------------------------------------------

void run_sensor(key_t key, Ethernet::Address mac) {
    // bloqueia SIGUSR1 para evitar handler com ponteiro inválido herdado do pai
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    std::cout << "[sensor] pid=" << getpid() << "\n";

    // cria os objetos do componente — ShmEngine inicializa _instance_sem correto
    NIC<ShmEngine>  nic(key, mac);
    Protocol        protocol(&nic);
    Communicator    comm(&protocol, Protocol::Address{mac, Ports::SENSOR});

    // agora o handler aponta para o semáforo correto — pode desbloquear
    sigprocmask(SIG_UNBLOCK, &mask, nullptr);

    while(!g_stop) {
        Message msg;
        if(comm.receive(&msg)) {
            std::string txt(static_cast<char*>(msg.data()), msg.size());
            std::cout << "[sensor] recebeu: '" << txt << "'\n";

            // ecoa de volta para o gateway
            Message resp;
            std::string reply = "sensor-ack: " + txt;
            std::memcpy(resp.data(), reply.c_str(), reply.size());
            resp.set_size(reply.size());
            comm.share(&resp, Ports::GATEWAY);
        }
    }
}

void run_actuator(key_t key, Ethernet::Address mac) {
    // bloqueia SIGUSR1 para evitar handler com ponteiro inválido herdado do pai
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    std::cout << "[actuator] pid=" << getpid() << "\n";

    // cria os objetos do componente — ShmEngine inicializa _instance_sem correto
    NIC<ShmEngine>  nic(key, mac);
    Protocol        protocol(&nic);
    Communicator    comm(&protocol, Protocol::Address{mac, Ports::ACTUATOR});

    // agora o handler aponta para o semáforo correto — pode desbloquear
    sigprocmask(SIG_UNBLOCK, &mask, nullptr);

    while(!g_stop) {
        Message msg;
        if(comm.receive(&msg)) {
            std::string txt(static_cast<char*>(msg.data()), msg.size());
            std::cout << "[actuator] recebeu: '" << txt << "'\n";
        }
    }
}

void run_powertrain(key_t key, Ethernet::Address mac) {
    // bloqueia SIGUSR1 para evitar handler com ponteiro inválido herdado do pai
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    std::cout << "[powertrain] pid=" << getpid() << "\n";

    // cria os objetos do componente — ShmEngine inicializa _instance_sem correto
    NIC<ShmEngine>  nic(key, mac);
    Protocol        protocol(&nic);
    Communicator    comm(&protocol, Protocol::Address{mac, Ports::POWERTRAIN});

    // agora o handler aponta para o semáforo correto — pode desbloquear
    sigprocmask(SIG_UNBLOCK, &mask, nullptr);

    while(!g_stop) {
        Message msg;
        if(comm.receive(&msg)) {
            std::string txt(static_cast<char*>(msg.data()), msg.size());
            std::cout << "[powertrain] recebeu: '" << txt << "'\n";
        }
    }
}

// ---------------------------------------------------------------------------
// Modos do processo pai (gateway)
// ---------------------------------------------------------------------------

// Ecoa "ping" → "pong" via raw socket (inter-VM)
// precisa rodar make vm_responder antes de make vm_rtt
void run_responder(Gateway& gw) {
    std::cout << "[responder] aguardando ping...\n";
    while(!g_stop) {
        Message msg;
        if(gw.receive(msg)) {
            std::string txt(static_cast<char*>(msg.data()), msg.size());
            if(txt == "ping") {
                Message resp;
                std::string pong = "pong";
                std::memcpy(resp.data(), pong.c_str(), pong.size());
                resp.set_size(pong.size());
                gw.send(resp);
                std::cout << "[responder] respondeu pong" << "\n";
            }
        }
    }
}

// Mede RTT inter-VM via raw socket
void run_rtt(Gateway& gw) {
    long total = 0;
    int  count = 0;
    while(!g_stop) {
        Message msg;
        std::string txt = "ping";
        std::memcpy(msg.data(), txt.c_str(), txt.size());
        msg.set_size(txt.size());

        auto start = std::chrono::high_resolution_clock::now();
        gw.send(msg);

        while(true) {
            Message resp;
            gw.receive(resp);
            std::string r(static_cast<char*>(resp.data()), resp.size());
            if(r == "pong") break;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        total += rtt.count();
        count++;
        std::cout << "[rtt] amostra=" << count
                  << " rtt=" << rtt.count() << "us"
                  << " media=" << total / count << "us\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Modo normal: gateway envia para componentes locais via share()
// e também para a rede via send(), enquanto recebe de ambos
void run_normal(Gateway& gw) {
    std::thread net_sender;
    std::thread shm_sender;
    // thread que envia periodicamente para a rede (inter-VM)
    net_sender = std::thread([&gw]() {
        int count = 0;
        while(!g_stop) {
            Message msg;
            std::string txt = "rede-msg-" + std::to_string(count++);
            std::memcpy(msg.data(), txt.c_str(), txt.size());
            msg.set_size(txt.size());
            bool ok = gw.send(msg);
            std::cout << "[gateway/net] enviou '" << txt
                      << "' ok=" << ok << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    });

    // thread que envia periodicamente para componentes locais (intra-VM)
    shm_sender = std::thread([&gw]() {
        int count = 0;
        while(!g_stop) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            Message msg;
            std::string txt = "local-msg-" + std::to_string(count++);
            std::memcpy(msg.data(), txt.c_str(), txt.size());
            msg.set_size(txt.size());
            gw.share(msg, Ports::SENSOR);
            gw.share(msg, Ports::ACTUATOR);
            bool ok = gw.share(msg, Ports::POWERTRAIN);
            std::cout << "[gateway/shm] compartilhou '" << txt
                      << "' ok=" << ok << "\n";
        }
    });

    // loop principal: recebe de qualquer origem (rede ou shm)
    while(!g_stop) {
        Message msg;
        if(gw.receive(msg)) {
            size_t sz = msg.size();
            if(sz > 0 && sz <= Message::MAX_SIZE) {
                std::string txt(static_cast<char*>(msg.data()), sz);
                std::cout << "[gateway] recebeu: '" << txt << "'\n";
            }
        }
    }

    // saída limpa
    if(net_sender.joinable()) net_sender.join();
    if(shm_sender.joinable()) shm_sender.join();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    std::cout << "ESTAMOS TESTANDO PARADA POR SINAL!!!\n";
    setup_signals();

    int timeout_sec = (argc > 3) ? std::stoi(argv[3]) : 120; // timeout padrão de 10s para testes
    alarm(timeout_sec); // para evitar travar indefinidamente durante testes

    const std::string iface = (argc > 1) ? argv[1] : "eth0";
    const std::string modo  = (argc > 2) ? argv[2] : "normal";

    // ---------- fork dos componentes ANTES de construir objetos de rede ----------
    // Regra: nenhum ShmEngine/RawSocketEngine deve existir antes do fork.
    // Os filhos recebem key e mac via argv do processo pai — mas como ainda
    // não temos o Vehicle aqui, usamos um fork simples com exec posterior.
    // Por ora, os filhos recebem key e mac como argumentos: key mac port.
    // O pai constrói o Vehicle, obtém key+mac, e relança os filhos com exec.

    // Passo 1: fork filhos como placeholders — eles vão re-exec com args corretos
    // (abordagem mais simples: pai cria Vehicle, fork filhos passando key+mac)

    // Cria Vehicle no pai para obter key e mac
    Vehicle v(iface);
    key_t             key = v.shm_key();
    Ethernet::Address mac = v.mac_address();
    Gateway&          gw  = v.gateway();

    std::cout << "[gateway] pid=" << getpid() << " mac=";
    for(int i = 0; i < 6; i++) {
        if(i) std::cout << ":";
        std::cout << std::hex << static_cast<int>(mac.bytes[i]);
    }
    std::cout << std::dec << "\n";

    // Passo 2: fork dos componentes — cada filho constrói seus objetos após fork
    std::vector<pid_t> children;

    auto spawn = [&](auto fn) {
        pid_t pid = fork();
        if(pid == 0) {
            fn(key, mac);   // filho inicializa seus próprios objetos aqui
            exit(0);
        }
        if(pid < 0) { perror("fork"); exit(1); }
        children.push_back(pid);
    };

    spawn(run_sensor);
    spawn(run_actuator);
    spawn(run_powertrain);

    // pequena pausa para os filhos registrarem na shm antes do pai enviar
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // ---------- processo pai vira gateway ----------
    if(modo == "responder") {
        run_responder(gw);
    } else if(modo == "rtt") {
        run_rtt(gw);
    } else {
        run_normal(gw);
    }

    g_stop = 1; // sinaliza para os filhos pararem
    
    std::cout << "SHUTDOWN INICIADO, AGUARDANDO FILHOS TERMINAREM...\n";

    for (pid_t pid : children)
        kill(pid, SIGTERM); // garante que filhos sejam terminados

    for(pid_t pid : children)
        waitpid(pid, nullptr, 0);

    return 0;
}