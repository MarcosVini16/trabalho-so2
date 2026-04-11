// vehicle.hpp
#pragma once
#include <vector>
#include <memory>
#include <thread>
#include "components/component.hpp"
#include "components/gateway.hpp"
#include "utils/ports.hpp"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "ethernet.hpp"

/*
 * Representação de um veículo autônomo, 
 * contendo seus componentes e o gateway para comunicação externa.
*/

#include "ethernet.hpp"
class Vehicle {

    using Frame = Ethernet::Frame;
public:
    Vehicle(const std::string& iface, Ethernet::Address mac)
        : _mac(mac),
          _gateway(iface)
    {
        // Inicializa memória compartilhada e semáforos para comunicação entre componentes, se necessário.
        key_path = "/tmp/vehicle_shm_key"; // Caminho para gerar a chave IPC
        project_id = 1; // ID do projeto para gerar a chave IPC (0-255)

        key = ftok(key_path, project_id); // Gera a chave IPC
        if (key == -1) {
            perror("ftok");
            exit(EXIT_FAILURE);
        }

        // Cria a memória compartilhada
        shmid = shmget(key, sizeof(Frame), IPC_CREAT | 0666);
        if (shmid == -1) {
            perror("shmget");
            exit(EXIT_FAILURE);
        }

        // Anexa a memória compartilhada ao processo
        Frame* shm_ptr = static_cast<Frame*>(shmat(shmid, nullptr, 0));
        if (shm_ptr == reinterpret_cast<void*>(-1)) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }

        // Cria o semáforo        
        semid = semget(key, 1, IPC_CREAT | 0666);
        if (semid == -1) {
            perror("semget");
            exit(EXIT_FAILURE);
        }

        // Inicializa o semáforo com valor 1 (indica que a memória compartilhada está disponível)
        if (semctl(semid, 0, SETVAL, 1) == -1) {
            perror("semctl");
            exit(EXIT_FAILURE);
        }

    }

    /*
     * Adiciona um componente ao veículo.
     * C deve ser uma classe derivada de Component.
     * Os argumentos args são passados para o construtor do componente.
     */
    template<typename C, typename... Args>
    C& add_component(Args&&... args) {
        auto ptr = std::make_unique<C>(std::forward<Args>(args)...);
        C& ref = *ptr;
        _components.push_back(std::move(ptr));
        return ref;
    }

    
    void run() {
        
    }

    Gateway& gateway() { return _gateway; }

private:
    Ethernet::Address _mac; // Endereço MAC do veículo (pode ser usado para identificação)
    Gateway                        _gateway; // O gateway para comunicação externa
    std::vector<std::unique_ptr<Component>> _components; // Componentes do veículo (sensores, atuadores, etc.)
    std::vector<std::thread> _threads; // Threads para rodar os componentes, se necessário
    key_t key; // Chave IPC para memória compartilhada e semáforos (gerada a partir de um caminho e um ID de projeto)
    int shmid; // ID da memória compartilhada (retornado por shmget)
    char* key_path; // Caminho para gerar a chave IPC (usado em ftok)
    int project_id; // ID do projeto para gerar a chave IPC (usado em ftok)
    int semid; // ID do semáforo (retornado por semget)

};