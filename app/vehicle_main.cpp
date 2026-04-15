// app/vehicle_main.cpp
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>
#include "../include/vehicle.hpp"
#include "../include/utils/ports.hpp"

// cada componente filho só dorme — comunicação intra-VM fica para depois
void run_sensor() {
    std::cout << "[sensor] processo iniciado (pid=" << getpid() << ")\n";
    while(true) pause();
}

void run_actuator() {
    std::cout << "[actuator] processo iniciado (pid=" << getpid() << ")\n";
    while(true) pause();
}

void run_powertrain() {
    std::cout << "[powertrain] processo iniciado (pid=" << getpid() << ")\n";
    while(true) pause();
}

void run_responder(Vehicle& v) {
    Gateway& gateway = v.gateway();
    std::cout << "[responder] aguardando mensagens...\n";
    while(true) {
        Message msg;
        if(gateway.receive(msg)) {
            std::string txt(static_cast<char*>(msg.data()), msg.size());
            std::cout << "[responder] recebeu: '" << txt << "'\n";
            if(txt == "ping") {
                Message resp;
                std::string pong = "pong";
                std::memcpy(resp.data(), pong.c_str(), pong.size());
                resp.set_size(pong.size());
                gateway.send(resp);
            }
        }
    }
}

void run_rtt(Vehicle& v) {
    Gateway& gateway = v.gateway();
    long total = 0;
    int  count = 0;
    while(true) {
        Message msg;
        std::string txt = "ping";
        std::memcpy(msg.data(), txt.c_str(), txt.size());
        msg.set_size(txt.size());

        auto start = std::chrono::high_resolution_clock::now();
        gateway.send(msg);

        while(true) {
            Message resp;
            gateway.receive(resp);
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

int main(int argc, char* argv[]) {
    const std::string iface = (argc > 1) ? argv[1] : "enp0s1";
    const std::string modo  = (argc > 2) ? argv[2] : "normal";

    // faz fork dos componentes ANTES de construir qualquer objeto de rede
    // (componentes ainda não usam shm — só dormem)
    std::vector<pid_t> children;
    auto spawn = [&](auto fn) {
        pid_t pid = fork();
        if(pid == 0) { fn(); exit(0); }
        children.push_back(pid);
    };

    spawn(run_sensor);
    spawn(run_actuator);
    spawn(run_powertrain);

    // processo pai constrói o Vehicle e vira gateway
    Vehicle v(iface);
    Gateway& gateway = v.gateway();

    std::cout << "[gateway] pid=" << getpid() << " mac=";
    for(int i = 0; i < 6; i++) {
        if(i) std::cout << ":";
        std::cout << std::hex
                  << static_cast<int>(gateway.address().paddr.bytes[i]);
    }
    std::cout << std::dec << "\n";

    if(modo == "responder") {
        run_responder(v);
    }
    else if(modo == "rtt") {
        run_rtt(v);
    }
    else {
        // modo normal: sender em thread separada + receiver no loop principal
        std::thread sender([&gateway]() {
            int count = 0;
            while(true) {
                Message msg;
                std::string txt = "Hello from gateway! Count: "
                                  + std::to_string(count++);
                std::memcpy(msg.data(), txt.c_str(), txt.size());
                msg.set_size(txt.size());
                bool ok = gateway.send(msg);
                std::cout << "[sender] msg " << count
                          << " ok=" << ok << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        while(true) {
            Message msg;
            if(gateway.receive(msg)) {
                size_t sz = msg.size();
                if(sz > 0 && sz <= Message::MAX_SIZE) {
                    std::string txt(static_cast<char*>(msg.data()), sz);
                    std::cout << "[gateway] recebeu: " << txt << "\n";
                }
            }
        }

        sender.join();
    }

    for(pid_t pid : children)
        waitpid(pid, nullptr, 0);

    return 0;
}