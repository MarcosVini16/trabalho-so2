#include "../../include/engine/shm_engine.hpp"

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <iostream>

// union exigida pelo semctl
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
// glibc já define
#else
union semun {
    int              val;
    struct semid_ds* buf;
    unsigned short*  array;
};
#endif

// ------------------------------------------------------------
// ctor / dtor
// ------------------------------------------------------------

ShmEngine::ShmEngine() {
    // garante que o path existe para ftok
    int fd = open(SHM_PATH, O_CREAT | O_RDWR, 0666);
    if (fd < 0) throw std::runtime_error("shm: open key path falhou");
    close(fd);

    _attach_shm();
    _attach_sem();

    // MAC virtual: 02:00:<pid bytes>
    pid_t pid = getpid();
    _address.bytes[0] = 0x02;
    _address.bytes[1] = 0x00;
    _address.bytes[2] = (pid >> 24) & 0xff;
    _address.bytes[3] = (pid >> 16) & 0xff;
    _address.bytes[4] = (pid >> 8)  & 0xff;
    _address.bytes[5] =  pid        & 0xff;

    _reader_idx = _register_reader();
    _cursor     = _hdr->next_seq.load();   // ignora histórico; só escuta dali pra frente

    _running = true;
    _thread  = std::thread([this]{ _receive_loop(); });
}

ShmEngine::~ShmEngine() {
    _running = false;

    // destravar o P(FULL[i]) da thread com um V fake
    if (_reader_idx >= 0) {
        _sem_v(SEM_FULL_BASE + _reader_idx);
    }
    if (_thread.joinable()) _thread.join();

    if (_reader_idx >= 0) _unregister_reader();

    if (_hdr) shmdt(_hdr);
    // não removemos shm/sem: outros processos podem estar usando.
    // Limpeza explícita via ipcrm ou script, se desejado.
}

// ------------------------------------------------------------
// setup de shm e sem
// ------------------------------------------------------------

void ShmEngine::_attach_shm() {
    key_t key = ftok(SHM_PATH, SHM_PROJ_ID);
    if (key == -1) throw std::runtime_error("shm: ftok falhou");

    _shmid = shmget(key, sizeof(Header), IPC_CREAT | 0666);
    if (_shmid < 0) throw std::runtime_error("shm: shmget falhou");

    void* addr = shmat(_shmid, nullptr, 0);
    if (addr == (void*)-1) throw std::runtime_error("shm: shmat falhou");
    _hdr = static_cast<Header*>(addr);

    // inicialização once: primeiro processo zera o header
    uint32_t expected = 0;
    if (_hdr->initialized.compare_exchange_strong(expected, 1)) {
        _hdr->next_seq.store(1);
        _hdr->n_readers.store(0);
        for (auto& r : _hdr->reader_alive) r.store(0);
        std::memset(_hdr->slots, 0, sizeof(_hdr->slots));
        _hdr->initialized.store(INIT_MAGIC);
    } else {
        // espera inicialização concorrente terminar
        while (_hdr->initialized.load() != INIT_MAGIC) {
            std::this_thread::yield();
        }
    }
}

void ShmEngine::_attach_sem() {
    key_t key = ftok(SHM_PATH, SHM_PROJ_ID + 1);
    if (key == -1) throw std::runtime_error("shm: ftok sem falhou");

    // tenta criar exclusivamente; se falhar com EEXIST, só pega o id
    _semid = semget(key, SEM_COUNT, IPC_CREAT | IPC_EXCL | 0666);
    if (_semid >= 0) {
        // criado agora: inicializa MUTEX=1 e FULL[i]=0
        union semun arg;
        unsigned short vals[SEM_COUNT];
        vals[SEM_MUTEX] = 1;
        for (int i = 0; i < (int)MAX_READERS; ++i) vals[SEM_FULL_BASE + i] = 0;
        arg.array = vals;
        if (semctl(_semid, 0, SETALL, arg) < 0)
            throw std::runtime_error("shm: semctl SETALL falhou");
    } else if (errno == EEXIST) {
        _semid = semget(key, SEM_COUNT, 0666);
        if (_semid < 0) throw std::runtime_error("shm: semget falhou");
    } else {
        throw std::runtime_error("shm: semget IPC_CREAT falhou");
    }
}

void ShmEngine::_sem_p(int semnum) {
    struct sembuf op{ (unsigned short)semnum, -1, 0 };
    while (semop(_semid, &op, 1) < 0) {
        if (errno == EINTR) continue;
        throw std::runtime_error("shm: semop P falhou");
    }
}

void ShmEngine::_sem_v(int semnum) {
    struct sembuf op{ (unsigned short)semnum, +1, 0 };
    while (semop(_semid, &op, 1) < 0) {
        if (errno == EINTR) continue;
        throw std::runtime_error("shm: semop V falhou");
    }
}

// ------------------------------------------------------------
// registro de leitor
// ------------------------------------------------------------

int ShmEngine::_register_reader() {
    for (int i = 0; i < (int)MAX_READERS; ++i) {
        uint32_t expected = 0;
        if (_hdr->reader_alive[i].compare_exchange_strong(expected, 1)) {
            _hdr->n_readers.fetch_add(1);
            return i;
        }
    }
    throw std::runtime_error("shm: sem slots de leitor");
}

void ShmEngine::_unregister_reader() {
    _hdr->reader_alive[_reader_idx].store(0);
    _hdr->n_readers.fetch_sub(1);
    // drena o semáforo do leitor pra não deixar contagem pendurada
    union semun arg; arg.val = 0;
    semctl(_semid, SEM_FULL_BASE + _reader_idx, SETVAL, arg);
}

// ------------------------------------------------------------
// send / receive
// ------------------------------------------------------------

int ShmEngine::_send(const void* buf, size_t len) {
    if (len > sizeof(Ethernet::Frame)) return -1;

    _sem_p(SEM_MUTEX);

    uint32_t seq = _hdr->next_seq.fetch_add(1);
    uint32_t idx = seq % RING_SLOTS;
    Slot& slot = _hdr->slots[idx];

    slot.len = (uint32_t)len;
    std::memcpy(slot.frame, buf, len);
    // seq por último: leitor usa isso como "commit"
    std::atomic_thread_fence(std::memory_order_release);
    slot.seq = seq;

    // acorda todos os leitores ativos
    for (int i = 0; i < (int)MAX_READERS; ++i) {
        if (_hdr->reader_alive[i].load()) {
            _sem_v(SEM_FULL_BASE + i);
        }
    }

    _sem_v(SEM_MUTEX);
    return (int)len;
}

void ShmEngine::_receive_loop() {
    while (_running) {
        _sem_p(SEM_FULL_BASE + _reader_idx);
        if (!_running) break;

        // drena tudo que está disponível: pode ter chegado mais de uma msg
        while (_cursor < _hdr->next_seq.load()) {
            uint32_t seq = _cursor;
            Slot& slot = _hdr->slots[seq % RING_SLOTS];

            // se o writer já deu a volta e sobrescreveu, o seq no slot
            // será > seq que queríamos → perdemos mensagens, pula pra frente
            std::atomic_thread_fence(std::memory_order_acquire);
            if (slot.seq != seq) {
                // leitor ficou pra trás; ressincroniza
                _cursor = _hdr->next_seq.load();
                break;
            }

            uint8_t tmp[sizeof(Ethernet::Frame)];
            uint32_t len = slot.len;
            if (len > sizeof(tmp)) len = sizeof(tmp);
            std::memcpy(tmp, slot.frame, len);

            _cursor++;
            _handle(tmp, len);
        }
    }
}

void ShmEngine::_handle(void* /*buf*/, size_t /*len*/) {
    // Sobrescrito pela NIC<ShmEngine> via override na classe derivada.
    // No ShmEngine puro, não faz nada.
}