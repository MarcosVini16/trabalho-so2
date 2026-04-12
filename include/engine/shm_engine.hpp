#pragma once
#include "engine.hpp"
#include "../ethernet.hpp"
#include <atomic>
#include <thread>
#include <string>
#include <unistd.h>

/*
 * ShmEngine — barramento broadcast em memória compartilhada (System V IPC).
 *
 * Modelo:
 *   - Um segmento shmget contendo um ring de N slots de frames Ethernet.
 *   - Cada processo se registra como "leitor" e recebe seu próprio semáforo
 *     FULL, garantindo broadcast verdadeiro: um V do writer solta todos.
 *   - Writer usa um semáforo MUTEX binário para serializar escritas e então
 *     executa V em todos os FULL[i] dos leitores ativos.
 *   - Cada leitor tem um cursor local e faz P(FULL[i]) para aguardar.
 *   - Slots são sobrescritos em ring. Leitor lento perde mensagens (drop),
 *     comportamento coerente com um barramento físico.
 *
 * Endereçamento:
 *   - read_address() deriva um MAC virtual do PID, com primeiro byte 0x02
 *     (locally-administered) para não colidir com MACs reais.
 */
class ShmEngine : public Engine {
public:
    static constexpr unsigned int RING_SLOTS   = 64;
    static constexpr unsigned int MAX_READERS  = 16;
    static constexpr const char*  SHM_PATH     = "/tmp/so2_shm_engine";
    static constexpr int          SHM_PROJ_ID  = 0x42;

    ShmEngine();
    ~ShmEngine();

    // MAC virtual único por processo, derivado do PID
    Ethernet::Address read_address() const { return _address; }

    // expõe pro NIC (mesma assinatura do RawSocketEngine)
    Ethernet::Address address() const { return _address; }

protected:
    int  _send(const void* buf, size_t len) override;
    void _handle(void* buf, size_t len) override;   // no-op; quem chama é a NIC via override

    void _receive_loop();

private:
    struct Slot {
        uint32_t seq;
        uint32_t len;
        uint8_t  frame[sizeof(Ethernet::Frame)];
    };

    struct Header {
        std::atomic<uint32_t> initialized;   // magic de init (0 = não inicializado)
        std::atomic<uint32_t> next_seq;      // próximo seq do writer
        std::atomic<uint32_t> n_readers;     // leitores ativos
        std::atomic<uint32_t> reader_alive[MAX_READERS]; // 1 se o slot de leitor está ocupado
        Slot slots[RING_SLOTS];
    };

    static constexpr uint32_t INIT_MAGIC = 0xC0FFEE42;

    // --- helpers System V ---
    void _attach_shm();
    void _attach_sem();
    int  _register_reader();     // pega um índice livre em reader_alive[]
    void _unregister_reader();

    void _sem_p(int semnum);
    void _sem_v(int semnum);

    // semáforo 0 = MUTEX do writer; 1..MAX_READERS = FULL[i] por leitor
    static constexpr int SEM_MUTEX     = 0;
    static constexpr int SEM_FULL_BASE = 1;
    static constexpr int SEM_COUNT     = 1 + MAX_READERS;

    int      _shmid = -1;
    int      _semid = -1;
    Header*  _hdr   = nullptr;
    int      _reader_idx = -1;
    uint32_t _cursor = 0;          // próximo seq que este leitor quer ler

    Ethernet::Address _address;
    std::thread       _thread;
    std::atomic<bool> _running{false};
};