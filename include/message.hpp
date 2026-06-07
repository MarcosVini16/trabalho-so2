#pragma once
#include <cstdint>
#include <cstring>
#include <cstddef>
#include "ethernet.hpp"

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
        _src = src;
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

    Ethernet::Address src() const { return _src; }
    void set_src(Ethernet::Address addr) { _src = addr; }

    uint8_t origin() const { return _origin & 0x3; }
    void set_origin(uint8_t q) { _origin = q & 0x3; }

    uint32_t data_type() const { return _data_type; }
    void set_data_type(uint32_t t) { _data_type = t; }

    uint8_t msg_type() const { return _msg_type; } 
    void set_msg_type(uint8_t t) { _msg_type = t; }

    private:

    uint8_t _origin = 0;
    uint8_t      _data[MAX_SIZE];
    unsigned int _size;
    Ethernet::Address _src; // Para armazenar o endereço de origem (caso seja necessário para respostas)
    uint64_t timestamp; // Momento de envio da mensagem
    uint32_t _data_type; // Campo para indicar o tipo de dado (Unit Code do SmartData), se necessário para processamento posterior
    uint8_t _msg_type;   // Campo para indicar o tipo de mensagem (0 = Interest, 1 = Response, 2 = PTP), se necessário para processamento posterior
    
};