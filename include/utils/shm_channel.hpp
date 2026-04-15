#pragma once
#include "../ethernet.hpp"
#include <cstdint>
#include <cstddef>
#include <sys/types.h>

/*
 * Estrutura de dados para comunicação entre processos usando memória compartilhada.
 * Implementa um canal de comunicação onde um processo pode escrever frames Ethernet e múltiplos processos podem ler esses frames de forma independente.
 * Cada leitor tem seu próprio "head" para acompanhar quais frames já leu, e o escritor tem um "tail" para indicar onde escrever o próximo frame.
*/
struct ShmChannel {
    static const int N     = 8; // slots de frames
    static const int PROCS = 8; // máximo de leitores simultâneos

    // --- escrita ---
    uint32_t tail;               // próximo slot livre para escrita

    // --- leitores registrados ---
    pid_t    pids[PROCS];        // PID de cada leitor (0 = slot livre)
    uint32_t heads[PROCS];       // head de leitura individual por processo

    // --- fila de frames ---
    size_t   sizes[N];
    uint8_t  slots[N][sizeof(Ethernet::Frame)];

    /*
     * @brief Encontra o índice do processo no array de leitores registrados.
     * @param pid O PID do processo a ser encontrado.
     * @return O índice do processo no array, ou -1 se não encontrado.
    */
    int find(pid_t pid) const {
        for (int i = 0; i < PROCS; i++)
            if (pids[i] == pid) return i;
        return -1;
    }

    /*
     * @brief Registra este processo e retorna seu índice, ou -1 se cheio.
     * @param pid O PID do processo a ser registrado.
     * @return O índice do processo no array, ou -1 se o array estiver cheio.
     */
    int enroll(pid_t pid) {
        for (int i = 0; i < PROCS; i++) {
            if (pids[i] == 0) {
                pids[i]  = pid;
                heads[i] = tail; // começa a ler a partir de agora
                return i;
            }
        }
        return -1;
    }

    /*
     * @brief Remove este processo do array de leitores registrados.
     * @param pid O PID do processo a ser removido.
     */
    void leave(pid_t pid) {
        int i = find(pid);
        if (i >= 0) pids[i] = 0;
    }
};