#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
#include "ethernet.hpp"

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
    Message(const void* data, unsigned int size) : _size(size) {
        std::memcpy(_data, data, size);
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

    private:

    uint8_t      _data[MAX_SIZE];
    unsigned int _size;
    Ethernet::Address _src; // Para armazenar o endereço de origem (caso seja necessário para respostas)
    
};