#include "include/vehicle.hpp"
#include "include/components/sensor.hpp"
#include "include/components/actuator.hpp"
#include <string>
#include <iostream>
#include <sys/wait.h>

int main() {
    // descobre o MAC antes do fork — só o Vehicle/Gateway precisa da interface
    Vehicle v("enp0s1");
    Ethernet::Address mac = v.mac_address();
    key_t key = v.shm_key(); // expõe a chave para os filhos usarem

    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Fork falhou\n";
        return 1;
    }

    if (pid == 0) {
        // processo filho: cria e registra apenas o Sensor
        // cada componente registra o PID correto (do filho)
        NIC<ShmEngine> nic(key, mac);
        Protocol protocol(&nic);
        Communicator comm(&protocol,
            Protocol::Address{mac, Ports::SENSOR});

        Message msg;
        std::string text = "Hello from Sensor!";
        std::memcpy(msg.data(), text.c_str(), text.size());
        msg.set_size(text.size());
        comm.send(&msg);
        std::cout << "[filho] Sensor enviou\n";

    } else {
        // processo pai: cria e registra apenas o Actuator
        std::cout << "PID do filho: " << pid << "\n";
        NIC<ShmEngine> nic(key, mac);
        Protocol protocol(&nic);
        Communicator comm(&protocol,
            Protocol::Address{mac, Ports::ACTUATOR});

        Message msg;
        while (true) {
            if (comm.receive(&msg)) {
                std::string received(static_cast<char*>(msg.data()), msg.size());
                std::cout << "[pai] Actuator recebeu: " << received << "\n";
                break;
            }
        }

        wait(nullptr); // espera filho terminar
    }

    return 0;
}