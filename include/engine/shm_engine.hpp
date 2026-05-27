#pragma once
#include "engine.hpp"
#include <semaphore.h>
#include <thread>
#include <atomic>
#include <functional>
#include "../utils/shm_channel.hpp"
#include "../ethernet.hpp"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <semaphore.h>

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

        // tenta criar exclusivamente — só o primeiro processo inicializa
        _semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);
        if (_semid < 0) {
            // já existe — só abre
            _semid = semget(key, 1, 0666);
            if (_semid < 0) { perror("semget"); exit(1); }
        } else {
            // primeiro processo — inicializa mutex com valor 1
            _sem_op(MUTEX, +1);
        }

        // registra este processo como leitor
        _sem_op(MUTEX, -1);
        _idx = _ch->enroll(getpid());
        _sem_op(MUTEX, +1);
        
        if (_idx < 0) {
            std::cerr << "[shm] sem slots de leitor disponíveis\n";
            exit(1);
        }  

        sem_init(&_notify_sem, 0, 0);

        _instance_sem = &_sem;

        // instala handler de SIGUSR1 — será chamado quando
        // um produtor escrever um frame
        struct sigaction sa{};

        _instance = this;

        sa.sa_handler = &ShmEngine::_signal_handler;
        sigemptyset(&sa.sa_mask);
        sigaddset(&sa.sa_mask, SIGUSR1); // adiciona essa linha
        sa.sa_flags = SA_RESTART; 
        sigaction(SIGUSR1, &sa, nullptr);
        sem_init(&_sem, 0, 0);

        // guarda ponteiro para a instância atual para o handler
        // (limitação: um ShmEngine por processo)
        _instance = this;

        _reader_thread = std::thread([this]() {
            while (_running) {
                sem_wait(&_notify_sem);
                if (_running)
                    _try_read();
            }
        });
    }

    void start() {
        _thread = std::thread([this]{ _receive_loop(); });
    }


    ~ShmEngine() {
        signal(SIGUSR1, SIG_IGN);
        _running = false;
        sem_post(&_sem);
        if (_thread.joinable()) _thread.join();
        int i = _ch->find(getpid());
        if (i >= 0) _ch->pids[i] = 0;
        
        _running = false;
        sem_post(&_notify_sem); // acorda a thread para ela sair
        if (_reader_thread.joinable())
            _reader_thread.join();
        sem_destroy(&_notify_sem);

        shmdt(_ch);
        sem_destroy(&_sem);
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

    void set_handle_cb(std::function<void(void*, size_t)> cb) {
        _handle_cb = cb;
    }

protected:
    // chamado por NIC::send(buf) → E::_send(frame, size)
    int _send(const void* buf, size_t len) override {
        
        // verifica o tamanho do frame
        if (len > sizeof(Frame)) {
            std::cerr << "[shm] frame maior que MTU\n";
            return -1;
        }

        // entra na seção crítica (só um processo por vez pode escrever na fila)
        _sem_op(MUTEX, -1);

        // verifica se todos os leitores já leram o slot que será sobrescrito

        // para cada leitor registrado (exceto o próprio processo):
        for (int i = 0; i < ShmChannel::PROCS; i++) {
            if (_ch->pids[i] != 0 && _ch->pids[i] != getpid()) {
                uint32_t head;

                // fica repetindo até a fila ter espaço
                while (true) {
                    // lê heads[i] da memória (sem cache do compilador)
                    __atomic_load(&_ch->heads[i], &head, __ATOMIC_ACQUIRE);
                    // se tail (escritor) - head (leitor) > 8, tem espaço pra escrever
                    if (_ch->tail - head < ShmChannel::N) break;
                    // libera o mutex
                    _sem_op(MUTEX, +1);
                    // como não tem o que ele fazer, faz o processo abrir mão da CPU
                    std::this_thread::yield();
                    // readquire mutex pra próxima iteração (com próximo processo)
                    _sem_op(MUTEX, -1);
                }
            }
        }

        // MEMÓRIA COMPARTILHADA:
        // recalcula qual slot a tail vai usar
        uint32_t slot = _ch->tail % ShmChannel::N;
        // copia o frame pro slot na memória compartilhada
        std::memcpy(_ch->slots[slot], buf, len);
        // registra o tamanho do frame no slot
        _ch->sizes[slot] = len;
        // avança a tail (indica que o slot foi preenchido)
        // e o próximo frame vai para o seguinte
        _ch->tail++;

        // notifica que a memória compartilhada foi atualizada aos leitores
        for (int i = 0; i < ShmChannel::PROCS; i++) {
            pid_t pid = _ch->pids[i];
            // não notifica a si mesmo
            if (pid != 0 && pid != getpid())
                kill(pid, SIGUSR1);
        }

        // sai da seção crítica
        _sem_op(MUTEX, +1);

        return static_cast<int>(len);
    }

private:
    // handler de sinal — sem alocação dinâmica, sem locks (async-signal-safe)
    static void _signal_handler(int) {
        if (!_instance) return;
        sem_post(&_instance->_notify_sem);
    }

        static void _signal_handler(int) {
            sem_post(_instance_sem); // async-signal-safe
        }

    void _receive_loop() {
        while (_running) {
            sem_wait(&_sem);
            // drena
            while (true) {
                _sem_op(MUTEX, -1);
                uint32_t my_head = _ch->heads[_idx];
                uint32_t tail = _ch->tail;
                if (my_head == tail) {
                    _sem_op(MUTEX, +1);
                    break;
                }
                uint32_t slot = my_head % ShmChannel::N;
                size_t len = _ch->sizes[slot];
                static uint8_t tmp[sizeof(Frame)];
                std::memcpy(tmp, _ch->slots[slot], len);
                _ch->heads[_idx] = my_head + 1;
                _sem_op(MUTEX, +1);
                if (_handle_cb) _handle_cb(tmp, len);
            }
        }
    }

    void _sem_op(int sem_num, int op) {
    struct sembuf sb = {
        static_cast<unsigned short>(sem_num),
        static_cast<short>(op),
        0
    };
        while (semop(_semid, &sb, 1) < 0) {
            if (errno == EINTR) continue; // reinicia se interrompido por sinal
            perror("semop");
            exit(1);
        }
    }

    Ethernet::Address  _address;
    int                _shmid;
    int                _semid;
    int                _idx;      // índice deste processo em ShmChannel::heads[]
    ShmChannel*        _ch;
    sem_t _notify_sem;
    std::thread _reader_thread;
    std::atomic<bool> _running{true};

    // ponteiro global para o handler de sinal alcançar a instância
    static ShmEngine* _instance;
    static sem_t* _instance_sem;
};

// shm_engine.cpp
ShmEngine* ShmEngine::_instance = nullptr;

sem_t* ShmEngine::_instance_sem = nullptr;