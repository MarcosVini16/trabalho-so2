// app/vehicle_main.cpp
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
// Funções dos processos filhos (componentes intra-VM)
// Cada um constrói seus próprios objetos APÓS o fork — nunca antes.
// ---------------------------------------------------------------------------

void run_sensor(key_t key, Ethernet::Address mac) {
    std::cout << "[sensor] pid=" << getpid() << "\n";

    NIC<ShmEngine>  nic(key, mac);
    Protocol        protocol(&nic);
    Communicator    comm(&protocol, Protocol::Address{mac, Ports::SENSOR});

    while(true) {
        Message msg;
        if(comm.receive(&msg)) {
            std::string txt(static_cast<char*>(msg.data()), msg.size());
            std::cout << "[sensor] recebeu: '" << txt << "'\n";

            // ecoa de volta para o gateway via share (broadcast local)
            Message resp;
            std::string reply = "sensor-ack: " + txt;
            std::memcpy(resp.data(), reply.c_str(), reply.size());
            resp.set_size(reply.size());
            comm.share(&resp);
        }
    }
}

void run_actuator(key_t key, Ethernet::Address mac) {
    std::cout << "[actuator] pid=" << getpid() << "\n";

    NIC<ShmEngine>  nic(key, mac);
    Protocol        protocol(&nic);
    Communicator    comm(&protocol, Protocol::Address{mac, Ports::ACTUATOR});

    while(true) {
        Message msg;
        if(comm.receive(&msg)) {
            std::string txt(static_cast<char*>(msg.data()), msg.size());
            std::cout << "[actuator] recebeu: '" << txt << "'\n";
        }
    }
}

void run_powertrain(key_t key, Ethernet::Address mac) {
    std::cout << "[powertrain] pid=" << getpid() << "\n";

    NIC<ShmEngine>  nic(key, mac);
    Protocol        protocol(&nic);
    Communicator    comm(&protocol, Protocol::Address{mac, Ports::POWERTRAIN});

    while(true) {
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
void run_responder(Gateway& gw) {
    std::cout << "[responder] aguardando ping...\n";
    while(true) {
        Message msg;
        if(gw.receive(msg)) {
            std::string txt(static_cast<char*>(msg.data()), msg.size());
            if(txt == "ping") {
                Message resp;
                std::string pong = "pong";
                std::memcpy(resp.data(), pong.c_str(), pong.size());
                resp.set_size(pong.size());
                gw.send(resp);
            }
        }
    }
}

// Mede RTT inter-VM via raw socket
void run_rtt(Gateway& gw) {
    long total = 0;
    int  count = 0;
    while(true) {
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
    // thread que envia periodicamente para a rede (inter-VM)
    std::thread net_sender([&gw]() {
        int count = 0;
        while(true) {
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
    std::thread shm_sender([&gw]() {
        int count = 0;
        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            Message msg;
            std::string txt = "local-msg-" + std::to_string(count++);
            std::memcpy(msg.data(), txt.c_str(), txt.size());
            msg.set_size(txt.size());
            bool ok = gw.share(msg);
            std::cout << "[gateway/shm] compartilhou '" << txt
                      << "' ok=" << ok << "\n";
        }
    });

    // loop principal: recebe de qualquer origem (rede ou shm)
    while(true) {
        Message msg;
        if(gw.receive(msg)) {
            size_t sz = msg.size();
            if(sz > 0 && sz <= Message::MAX_SIZE) {
                std::string txt(static_cast<char*>(msg.data()), sz);
                std::cout << "[gateway] recebeu: '" << txt << "'\n";
            }
        }
    }

    net_sender.join();
    shm_sender.join();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
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

    for(pid_t pid : children)
        waitpid(pid, nullptr, 0);

    return 0;
}