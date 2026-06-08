// app/vehicle_main.cpp
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>
#include <string>
#include <fstream>
#include "../include/vehicle.hpp"
#include "../include/components/time_client.hpp"
#include "../include/utils/ports.hpp"
#include "../include/utils/stats.hpp"
#include "../include/components/CompA.hpp"
#include "../include/components/CompB.hpp"

// ---------------------------------------------------------------------------
// Sinal de parada total (visível para todos os filhos pós-fork)
// ---------------------------------------------------------------------------

volatile sig_atomic_t g_stop = 0;

extern "C" void on_stop(int) {
    g_stop = 1;
}

void setup_signals() {
    struct sigaction sa{};
    sa.sa_handler = on_stop;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGALRM, &sa, nullptr);
}

Stats g_stats;

// ---------------------------------------------------------------------------
// Funções de cada processo filho.
// Cada uma constrói seus objetos APÓS o fork — nunca antes.
// ---------------------------------------------------------------------------

// void run_sensor(key_t key, Ethernet::Address mac, const std::string& iface) {
//     //std::cout << "[sensor] pid=" << getpid() << "\n";
//     Sensor s(Protocol::Address{mac, Ports::SENSOR}, key, mac);
//     s.run();
//     //std::cout << "[sensor] saindo...\n";
// }

void run_compA(key_t key, Ethernet::Address mac, const std::string& iface) {
    //std::cout << "[compA] pid=" << getpid() << "\n";
    CompA a(key, mac);
    a.setup();
    while(!g_stop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    //std::cout << "[compA] saindo...\n";
}

void run_compB(key_t key, Ethernet::Address mac, const std::string& iface) {
    //std::cout << "[compB] pid=" << getpid() << "\n";
    CompB b(key, mac);
    b.setup();
    while(!g_stop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    //std::cout << "[compB] saindo...\n";
}

// void run_actuator(key_t key, Ethernet::Address mac, const std::string& iface) {
//     //std::cout << "[actuator] pid=" << getpid() << "\n";
//     Actuator a(Protocol::Address{mac, Ports::ACTUATOR}, key, mac);
//     a.run();
//     //std::cout << "[actuator] saindo...\n";
// }

void run_time_client(key_t key, Ethernet::Address mac, const std::string& iface) {
    //std::cout << "[time_client] pid=" << getpid() << "\n";
    TimeClient tc(Protocol::Address{mac, Ports::TIME_CLIENT}, key,  mac, iface);
    tc.run();
    //std::cout << "[time_client] saindo...\n";
}

// ---------------------------------------------------------------------------
// Modos do processo pai (gateway)
// ---------------------------------------------------------------------------

// Modo normal: gateway envia para componentes locais via share()
// e também para a rede via send(), enquanto recebe de ambos
void run_normal(Gateway& gw) {
    std::thread net_sender = std::thread([&gw]() {
        int count = 0;
        while(!g_stop) {
            Message msg;
            std::string txt = "rede-msg-" + std::to_string(count++);
            std::memcpy(msg.data(), txt.c_str(), txt.size());
            msg.set_size(txt.size());
            gw.send(msg);
            std::cout << "[gateway/net] enviou '" << txt << "\n";
                      //<< "' ok=" << ok << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        std::cout << "[gateway/net] saindo...\n";
    });

    std::thread shm_sender = std::thread([&gw]() {
        int count = 0;
        while(!g_stop) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            Message msg;
            std::string txt = "local-msg-" + std::to_string(count++);
            std::memcpy(msg.data(), txt.c_str(), txt.size());
            msg.set_size(txt.size());
            gw.share(msg, Ports::SENSOR);
            gw.share(msg, Ports::ACTUATOR);
            gw.share(msg, Ports::POWERTRAIN);
            std::cout << "[gateway/shm] compartilhou '" << txt << "\n";
                      //<< "' ok=" << ok << "\n";
        }
        std::cout << "[gateway/shm] saindo...\n";
    });

    while(!g_stop) {
        Message msg;
        if(gw.receive(msg)) {
            size_t sz = msg.size();
            if(sz > 0 && sz <= Message::MAX_SIZE) {
                std::string txt(static_cast<char*>(msg.data()), sz);
                std::cout << "[gateway] recebeu de quadrante=" 
                      << (int)msg.origin() << "\n";
                      //<< " msg='" << txt << "'\n";
            }
        }
    }

    std::cout << "[gateway] shutdown iniciado, aguardando threads...\n";
    if(net_sender.joinable()) net_sender.join();
    if(shm_sender.joinable()) shm_sender.join();
    std::cout << "[gateway] saindo...\n";
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {

    // no início do main, antes de tudo
    std::ifstream f("/proc/position");
    if (f) {
        unsigned int q;
        f >> q;
        std::cout << "[main] quadrante inicial: " << q << "\n";
    } else {
        std::cerr << "[main] ERRO: /proc/position nao encontrado\n";
    }

    std::cout << "=== Vehicle main (com TimeClient) ===\n";
    setup_signals();

    int timeout_sec = 30;
    std::string iface = "eth0";

    // argv[1] = iface (opcional)
    if (argc > 1 && argv[1][0] != '\0') {
        iface = argv[1];
    }

    // argv[2] = timeout (opcional, deve ser número)
    if (argc > 2 && argv[2][0] != '\0') {
        try {
            timeout_sec = std::stoi(argv[2]);
        } catch (const std::exception& e) {
            std::cerr << "[main] timeout inválido '" << argv[2] 
                    << "', usando default " << timeout_sec << "s\n";
        }
    }

    std::cout << "[main] iface=" << iface 
            << " timeout=" << timeout_sec << "s argc=" << argc << "\n";

    alarm(timeout_sec); // shutdown automático para testes

    // ----- Vehicle precisa existir no pai para gerar key+mac -----
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

    // ----- fork dos componentes -----
    std::vector<pid_t> children;

    auto spawn = [&](auto fn) {
        pid_t pid = fork();
        if(pid == 0) {
            fn(key, mac, iface);
            exit(0);
        }
        if(pid < 0) { perror("fork"); exit(1); }
        children.push_back(pid);
    };

    // spawn(run_sensor);
    // spawn(run_actuator);
    // spawn(run_time_client);
    spawn(run_compA);
    spawn(run_compB);
    spawn(run_time_client);

    // pequena pausa para os filhos registrarem na shm antes do pai enviar
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // ----- processo pai vira gateway no modo normal -----
    // run_normal(gw);

    while(!g_stop) {
    }

    g_stop = 1;
    std::cout << "[main] aguardando filhos terminarem...\n";

    for(pid_t pid : children)
        kill(pid, SIGTERM);

    for(pid_t pid : children)
        waitpid(pid, nullptr, 0);

    std::cout << "[main] todos os filhos saíram. Desligando VM.\n";
    sync();
    reboot(RB_POWER_OFF);
    return 0; // nunca alcançado
}