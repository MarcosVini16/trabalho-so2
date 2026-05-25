#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
#include "ethernet.hpp"


struct Origin {
    // endereço MAC (6 bytes)
    Ethernet::Address address;
    // quadrante (2 bits)
    uint8_t quadrant : 2;
} __attribute__((packed));

/*
 * Classe que representa uma mensagem genérica a ser enviada ou recebida.
 * Contém um buffer de dados, o tamanho dos dados válidos, o endereço de origem e um timestamp.
*/
class Message {
    public:

        // o tamanho maximo da mensagem
        // 1500 -> ethernet, < protocol = 1024
        static const unsigned int MAX_SIZE = 1024;

        // construtor que cria uma mensagem vazia
        Message() : _size(0) {
            std::memset(_data, 0, MAX_SIZE);
        }

        // construtor que cria com conteúdo
        Message(Ethernet::Address src, const void* data, unsigned int size, uint64_t ts) : _size(size), timestamp(ts) {
            std::memcpy(_data, data, size);
            _origin.address = src;
            _origin.quadrant = 0;
        }

        // retorna o ponteiro pro array
        // só leitura
        const void* data() const { return _data; }
        // ler e modificar
        void*       data()       { return _data; }

        // retorna quantos bytes válidos tem a mensagem
        // o communicator passa pra protocol pra saber quantos bytes copiar pro ethernet
        unsigned int size() const { return _size; }

        // o communicator chama isso depois de receber dados pra registrar quantos bytes chegaram. (pra saber quando o conteúdo válido termina)
        void set_size(unsigned int s) { _size = s; }

        uint64_t get_timestamp() const { return timestamp; }
        void set_timestamp(uint64_t ts) { timestamp = ts; }

        Origin origin() const { return _origin; }
        void set_origin(Ethernet::Address addr, uint8_t q) {
            _origin.address = addr;
            // mascara de 2 bits 00000011 que garante só 2 bits
            _origin.quadrant = q & 0x3;
        }
        Ethernet::Address src() const { return _origin.address; }
        uint8_t quadrant() const { return _origin.quadrant; }

    private:

        uint8_t      _data[MAX_SIZE];
        unsigned int _size;
        Origin _origin;
        uint64_t timestamp; // Momento de envio da mensagem
    
};