// nic.hpp
#pragma once
#include "nic_base.hpp"
#include <iostream>
#include <mutex>
#include <arpa/inet.h> // for htons and ntohs
#include <unistd.h> // for getpid
#include "protocol.hpp"
#include "../utils/position.hpp"
/*
    * A Network Interface Card (NIC) class that combines a specific Engine (E) with the NICBase interface.
    * Sends and receives Ethernet frames using the underlying Engine's capabilities, 
    * while also providing conditional observation for Protocols to register 
    * and receive notifications when frames with their EtherType are received.
*/
template<typename E>
class NIC : public NICBase,
            private E
{
public:
    template<typename... Args>
    NIC(Args&&... args) : E(std::forward<Args>(args)...) {
        _address = this->read_address();  // correto — método de instância
        Protocol::set_quadrant(Position::quadrant());
    }

    int send(Buffer* buf) override {
        return E::_send(buf->frame(), buf->size());
    }

    Buffer* alloc(Address dst, Protocol_Number prot,
                  unsigned int size) override
    {
        std::lock_guard<std::mutex> lock(_pool_mutex);
        for(auto& buf : _buffer) {
            if(buf.is_free()) {
                buf.allocate();
                buf.set_size(sizeof(Ethernet::Address) * 2 + sizeof(Ethernet::Protocol) + size);
                buf.frame()->dst = dst;
                buf.frame()->src = _address;
                //std::cout << "[nic] alloc EtherType=0x" 
          //<< std::hex << prot << std::dec << "\n";
                buf.frame()->type = htons(prot); // htons to convert protocol number to network byte order (correctly filter frames in the NIC)
                buf.set_owner(this); // associa o buffer a esta NIC para facilitar a liberação posterior
                return &buf;
            }
        }
        return nullptr;
    }

    void free(Buffer* buf) override {
        if(buf) {
            buf->release();
        }
    }

    int receive(Buffer* buf, Address* src,
            void* data, unsigned int size) override
    {
        if(!buf || buf->is_free()) return -1;
        if(src) *src = buf->frame()->src;
        size_t header_size = sizeof(Ethernet::Address) * 2 + sizeof(Ethernet::Protocol);
        size_t payload_size = buf->size() > header_size ? buf->size() - header_size : 0;
        unsigned int len = std::min(size, static_cast<unsigned int>(payload_size));
        std::memcpy(data, buf->frame()->data, len);
        return static_cast<int>(len);
    }

    void attach(Observer* obs, Protocol_Number prot) override {
        Observed::attach(obs, prot);
    }

    void detach(Observer* obs, Protocol_Number prot) override {
        Observed::detach(obs, prot);
    }

    Address address() const override { return _address; }

    Address expected_dst() const override {
        return E::expected_dst();
    }

private:
    void _handle(void* raw, size_t len) override {
        auto* frame = static_cast<Ethernet::Frame*>(raw);

        uint16_t etype = ntohs(frame->type);

        if (!Protocol::verifica_quadrante(frame)) {
            return;
        }

        std::lock_guard<std::mutex> lock(_pool_mutex);
        for(auto& buf : _buffer) {
            if(buf.is_free()) {
                buf.allocate();
                buf.set_size(len);
                // auto* phdr = reinterpret_cast<Protocol::Header*>(buf.frame()->data);
                // if (!Protocol::accept(phdr->src_port)) {
                //     buf.release();
                //     return;
                // }
                std::memcpy(buf.frame(), raw, len);

                uint16_t frame_etype = ntohs(buf.frame()->type);
                this->notify(frame_etype, &buf);
                // não libera aqui — o Protocol libera após o notify
                return;
            }
        }
        //std::cout << "[nic] pool esgotado, frame descartado\n";
    }

    Address                  _address;
    std::mutex               _pool_mutex; // protege o pool de buffers contra acesso concorrente
    Buffer  _buffer[NICBase::BUFFER_COUNT];
};