#include "include/vehicle.hpp"
#include "include/components/sensor.hpp"
#include "include/components/actuator.hpp"
#include <string>
#include <iostream>
#include <sys/wait.h>
#include <execinfo.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

int main() {
    signal(SIGSEGV, [](int) {
        void* buf[20];
        int n = backtrace(buf, 20);
        backtrace_symbols_fd(buf, n, 2);
        exit(1);
    });

    // pega MAC e chave sem criar o Vehicle completo
    Ethernet::Address mac;
    key_t key;
    {
        int fd = socket(AF_PACKET, SOCK_RAW, htons(0x8888));
        struct ifreq ifr{};
        std::strncpy(ifr.ifr_name, "enp52s0", IFNAMSIZ-1);
        ioctl(fd, SIOCGIFHWADDR, &ifr);
        std::memcpy(mac.bytes, ifr.ifr_hwaddr.sa_data, 6);
        close(fd);
        key_t k = 0;
        std::memcpy(&k, mac.bytes + 2, 4);
        key = k ? k : 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        std::cerr << "Fork falhou\n";
        return 1;
    }

    if (pid == 0) {
        sleep(1); // espera o pai registrar o Communicator
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
        _exit(0); // não chama destrutores
    } else {
        // bloqueia SIGUSR1 imediatamente — antes de criar qualquer objeto
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);
        sigprocmask(SIG_BLOCK, &mask, nullptr);
        
        signal(SIGIO, SIG_DFL);
        signal(SIGUSR1, SIG_DFL);

        std::cout << "PID do filho: " << pid << "\n";
        
        {
            NIC<ShmEngine> nic(key, mac);
            Protocol protocol(&nic);
            Communicator comm(&protocol,
                Protocol::Address{mac, Ports::ACTUATOR});

            // desbloqueia só para receber
            sigprocmask(SIG_UNBLOCK, &mask, nullptr);

            Message msg;
            while (true) {
                if (comm.receive(&msg)) {
                    std::string received(static_cast<char*>(msg.data()), msg.size());
                    std::cout << "[pai] Actuator recebeu: " << received << "\n";
                    break;
                }
            }

            // bloqueia antes de destruir
            sigprocmask(SIG_BLOCK, &mask, nullptr);
            wait(nullptr);
        }

        _exit(0);
    }
}