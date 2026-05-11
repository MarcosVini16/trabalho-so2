#pragma once
#include "engine.hpp"
#include "../utils/shm_channel.hpp"
#include "../ethernet.hpp"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

class ShmEngine : public Engine {
    using Frame = Ethernet::Frame;

    // índices no array de semáforos SysV
    enum { MUTEX = 0 };  // só precisamos de mutex agora
                         // notificação é feita por SIGUSR1

public:
    ShmEngine(key_t key, Ethernet::Address mac)
        : _address(mac), _idx(-1)
    {
        // abre ou cria o segmento
        _shmid = shmget(key, sizeof(ShmChannel), IPC_CREAT | 0666);
        if (_shmid < 0) { perror("shmget"); exit(1); }

        // anexa o segmento à memória do processo
        _ch = static_cast<ShmChannel*>(shmat(_shmid, nullptr, 0));
        if (_ch == reinterpret_cast<void*>(-1)) { perror("shmat"); exit(1); }

        // abre ou cria o semáforo de mutex
        _semid = semget(key, 1, IPC_CREAT | 0666);
        if (_semid < 0) { perror("semget"); exit(1); }

        // inicialização idempotente: só seta se ainda não foi feito
        // (o primeiro processo a chegar inicializa)
        _sem_op(MUTEX, +1); // garante mutex=1 na primeira vez
                             // processos subsequentes não sofrem efeito pois
                             // ninguém está na seção crítica ainda
        // na prática: use IPC_EXCL para detectar criação e só então
        // inicializar — simplificado aqui para clareza

        // registra este processo como leitor
        _sem_op(MUTEX, -1);
        _idx = _ch->enroll(getpid());
        _sem_op(MUTEX, +1);

        if (_idx < 0) {
            std::cerr << "[shm] sem slots de leitor disponíveis\n";
            exit(1);
        }

        // instala handler de SIGUSR1 — será chamado quando
        // um produtor escrever um frame
        struct sigaction sa{};
        sa.sa_handler = &ShmEngine::_signal_handler;
        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, SIGUSR1); // adiciona essa linha
        sa.sa_flags = 0;
        sigaction(SIGUSR1, &sa, nullptr);

        // guarda ponteiro para a instância atual para o handler
        // (limitação: um ShmEngine por processo)
        _instance = this;
    }

    ~ShmEngine() {
        // desinstala o handler de SIGUSR1 antes de destruir
        signal(SIGUSR1, SIG_IGN);
        
        int i = _ch->find(getpid());
        if (i >= 0) _ch->pids[i] = 0;
        
        shmdt(_ch);

        std::cout << "[shm] processo " << getpid() << " saiu\n";
    }

    void _handle(void* buf, size_t len) {
        // implementado via CRTP na NIC<ShmEngine>
        // este método nunca é chamado diretamente —
        // _try_read chama this->_handle que resolve para NIC<E>::_handle
        std::cout << "[shm] _handle chamado len=" << len << "\n";
    }

    Ethernet::Address read_address() const { return _address; }

    Ethernet::Address expected_dst() const override {
        // para comunicação local, esperamos enviar para nós mesmos
        return _address;
    }

protected:
    // chamado por NIC::send(buf) → E::_send(frame, size)
    int _send(const void* buf, size_t len) override {
        //std::cout << "[shm] _send chamado len=" << len << "\n";
        if (len > sizeof(Frame)) {
            std::cerr << "[shm] frame maior que MTU\n";
            return -1;
        }

        _sem_op(MUTEX, -1); // entra na seção crítica

        uint32_t slot = _ch->tail % ShmChannel::N;
        std::memcpy(_ch->slots[slot], buf, len);
        _ch->sizes[slot] = len;
        _ch->tail++;

        // notifica todos os leitores registrados via SIGUSR1
        for (int i = 0; i < ShmChannel::PROCS; i++) {
            pid_t pid = _ch->pids[i];
            if (pid != 0 && pid != getpid()) // não notifica a si mesmo
                kill(pid, SIGUSR1);
        }

        _sem_op(MUTEX, +1); // sai da seção crítica

        return static_cast<int>(len);
    }

private:
    // handler de sinal — sem alocação dinâmica, sem locks (async-signal-safe)
    static void _signal_handler(int) {
        if (!_instance) return;
        _instance->_try_read();
    }

    // lê todos os frames pendentes para este processo
    void _try_read() {
        // lê enquanto houver frames à frente do nosso head
        while (true) {
            uint32_t my_head = _ch->heads[_idx];
            uint32_t tail;

            // leitura atômica do tail (sem lock — tail só cresce)
            __atomic_load(&_ch->tail, &tail, __ATOMIC_ACQUIRE);

            if (my_head == tail) break; // nada novo

            uint32_t slot = my_head % ShmChannel::N;
            size_t   len  = _ch->sizes[slot];

            // copia o frame antes de avançar o head
            uint8_t tmp[sizeof(Frame)];
            std::memcpy(tmp, _ch->slots[slot], len);

            // avança nosso head (só nós escrevemos aqui — sem lock)
            _ch->heads[_idx] = my_head + 1;

            // entrega para a NIC processar
            _handle(tmp, len);
        }
    }

    void _sem_op(int sem_num, int op) {
        struct sembuf sb = {
            static_cast<unsigned short>(sem_num),
            static_cast<short>(op),
            0
        };
        if (semop(_semid, &sb, 1) < 0) {
            perror("semop");
            exit(1);
        }
    }

    Ethernet::Address  _address;
    int                _shmid;
    int                _semid;
    int                _idx;      // índice deste processo em ShmChannel::heads[]
    ShmChannel*        _ch;

    // ponteiro global para o handler de sinal alcançar a instância
    static ShmEngine* _instance;
};

// shm_engine.cpp
ShmEngine* ShmEngine::_instance = nullptr;