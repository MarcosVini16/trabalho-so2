// app/vehicle_main.cpp
#include <unistd.h>
#include <sys/wait.h>
#include <iostream>
#include <vector>
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

int main(int argc, char* argv[]) {
    const std::string iface = (argc > 1) ? argv[1] : "enp0s1";

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

    // thread de envio periódico
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

    // loop de recepção
    while(true) {
        Message msg;
        if(gateway.receive(msg)) {
            std::string txt(static_cast<char*>(msg.data()), msg.size());
            std::cout << "[gateway] recebeu: " << txt << "\n";
        }
    }

    sender.join();

    // aguarda filhos ao encerrar
    for(pid_t pid : children)
        waitpid(pid, nullptr, 0);

    return 0;
}