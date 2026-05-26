#pragma once
#include <cstdint>
#include <cstring>
#include "ethernet.hpp"

/*
 * Classe que representa uma mensagem genérica a ser enviada ou recebida.
 * Contém um buffer de dados, o tamanho dos dados válidos, o endereço de origem e um timestamp.
*/
class Message {
    public:

        // o tamanho maximo da mensagem
        static const unsigned int MAX_SIZE = 1024;

        // construtor que cria uma mensagem vazia
        Message() : _size(0), _origin(0), _timestamp(0) {
            std::memset(_data, 0, MAX_SIZE);
        }

        const void* data() const { return _data; }
        void*       data()       { return _data; }

        unsigned int size() const { return _size; }
        void set_size(unsigned int s) { _size = s; }

        uint64_t get_timestamp() const { return _timestamp; }
        void set_timestamp(uint64_t ts) { _timestamp = ts; }

        uint8_t quadrant() const { return _origin; }
        void set_origin(uint8_t q) { _origin = q & 0x3; }

        // preenchido pelo communicator
        Ethernet::Address src() const { return _src_mac; }
        void set_src(Ethernet::Address mac) { _src_mac = mac; }

    private:

        uint8_t           _data[MAX_SIZE];
        unsigned int      _size;
        uint8_t           _origin;           // quadrante (0-3)
        uint64_t          _timestamp;
        Ethernet::Address _src_mac; 
    
};