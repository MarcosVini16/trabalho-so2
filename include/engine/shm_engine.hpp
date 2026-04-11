#pragma once
#include "engine.hpp"
#include "../ethernet.hpp"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

/*
 * Uma Engine para comunicação entre processos usando memória compartilhada.
 * Cada componente do veículo (sensor, atuador, etc.) pode usar essa engine para enviar e receber mensagens de outros componentes do mesmo veículo.
 */
class ShmEngine : public Engine {
    using Frame = Ethernet::Frame;
public:
    ShmEngine(int shmid, int semid) : shmid(shmid), semid(semid) {
        // Anexa a memória compartilhada ao processo
        shm_ptr = static_cast<Frame*>(shmat(shmid, nullptr, 0));
        if (shm_ptr == reinterpret_cast<void*>(-1)) {
            perror("shmat");
            exit(EXIT_FAILURE);
        }
    }
    ~ShmEngine();

    /*
     * @brief Aguarda a liberação do semáforo para acessar a memória compartilhada.
     */
    void sem_wait() {
        struct sembuf sb = {0, -1, 0}; // Decrementa o semáforo (P operation)
        if (semop(semid, &sb, 1) == -1) {
            perror("semop - wait");
            exit(EXIT_FAILURE);
        }
    }

    /*
     * @brief Libera o semáforo.
     * indicando que a memória compartilhada está disponível para leitura ou escrita.
     */
    void sem_signal() {
        struct sembuf sb = {0, 1, 0}; // Incrementa o semáforo (V operation)
        if (semop(semid, &sb, 1) == -1) {
            perror("semop - signal");
            exit(EXIT_FAILURE);
        }
    }
    // Função para descobrir endereço MAC
    void set_address(const Ethernet::Address& addr) { _address = addr; }
    Ethernet::Address address() const { return _address; }
    
protected:
    int  _send(const void* buf, size_t len) override {
        if (len > sizeof(Frame)) {
            std::cerr << "Error: Message size exceeds shared memory frame size\n";
            return -1;
        }

        // Escreve os dados na memória compartilhada
        std::memcpy(shm_ptr, buf, len);
        // Sinaliza que a mensagem está pronta para ser lida
        sem_signal();
        return len;
    }
    void _handle(void* buf, size_t len) override;
private:
    Ethernet::Address _address; // Endereço MAC do componente, usado para identificação
    int shmid; // ID da memória compartilhada
    int semid; // ID do semáforo
    Frame* shm_ptr; // Ponteiro para a memória compartilhada onde os frames são lidos/escritos
};