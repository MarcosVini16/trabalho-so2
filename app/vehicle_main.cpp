// app/vehicle_main.cpp
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <vector>
#include <chrono>
#include "../include/components/gateway.hpp"
#include "../include/utils/ports.hpp"

// cada componente roda isso no seu processo
void run_sensor() {
    std::cout << "[sensor] processo iniciado (pid=" << getpid() << ")\n";
    while(true)
        pause();  // dorme até receber sinal — Etapa 2 vai substituir isso
}

void run_actuator() {
    std::cout << "[actuator] processo iniciado (pid=" << getpid() << ")\n";
    while(true)
        pause();
}

void run_powertrain() {
    std::cout << "[powertrain] processo iniciado (pid=" << getpid() << ")\n";
    while(true)
        pause();
}

// VM respondedora — ecoa tudo que recebe de volta
void run_responder(Gateway& gateway) {
    while(true) {
        Message msg;
        if(gateway.receive(msg)) {
            std::string txt(static_cast<char*>(msg.data()), msg.size());
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

// VM medidora — envia ping e mede RTT
void run_rtt(Gateway& gateway) {
    long total = 0;
    int count = 0;
    while(true) {
        Message msg;
        std::string txt = "ping";
        std::memcpy(msg.data(), txt.c_str(), txt.size());
        msg.set_size(txt.size());

        auto start = std::chrono::high_resolution_clock::now();
        gateway.send(msg);

        // descarta mensagens até receber "pong"
        while(true) {
            gateway.receive(msg);
            std::string resp(static_cast<char*>(msg.data()), msg.size());
            //std::cout << "[rtt] recebeu: '" << resp << "' size=" << msg.size() << "\n";
            if(resp == "pong") break;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto rtt = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        total += rtt.count();
        count++;
        std::cout << "[rtt] amostra=" << count
                  << " rtt=" << rtt.count() << "us"
                  << " media=" << total/count << "us\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char* argv[]) {

    const std::string iface = (argc > 1) ? argv[1] : "enp0s1";
    const std::string modo  = (argc > 2) ? argv[2] : "normal";

    // cria processos filhos para cada componente
    std::vector<pid_t> children;

    auto spawn = [&](auto fn) {
        pid_t pid = fork();
        if(pid == 0) {
            fn();       // processo filho executa a função
            exit(0);
        }
        children.push_back(pid);
    };

    spawn(run_sensor);
    spawn(run_actuator);
    spawn(run_powertrain);

    // processo pai vira o gateway
    std::cout << "[gateway] processo pai (pid=" << getpid() << ")\n";

    Gateway gateway(iface);
    std::cout << "[gateway] NIC inicializada com endereço: " 
              << std::hex
              << static_cast<int>(gateway.address().paddr.bytes[0]) << ":"
              << static_cast<int>(gateway.address().paddr.bytes[1]) << ":"
              << static_cast<int>(gateway.address().paddr.bytes[2]) << ":"
              << static_cast<int>(gateway.address().paddr.bytes[3]) << ":"
              << static_cast<int>(gateway.address().paddr.bytes[4]) << ":"
              << static_cast<int>(gateway.address().paddr.bytes[5]) << std::dec
              << "\n";

    // modo respondedor — só ecoa, sem prints no caminho crítico
    if(modo == "responder") {
        run_responder(gateway);
    }
    // modo rtt — mede latência
    else if(modo == "rtt") {
        run_rtt(gateway);
    }
    // modo normal — comportamento original
    else {
        std::thread sender([&gateway]() {
            int count = 0;
            while(true) {
                Message msg;
                std::string txt = "Hello from gateway! Count: " + std::to_string(count++);
                std::memcpy(msg.data(), txt.c_str(), txt.size());
                msg.set_size(txt.size());
                bool ok = gateway.send(msg);
                std::cout << "[sender] enviou msg " << count 
                      << " resultado=" << ok << "\n";
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