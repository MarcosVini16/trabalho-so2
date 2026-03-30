#pragma once
#include "observe/concurrent.hpp"
#include "observe/conditional.hpp"
#include "message.hpp"
#include "utils/buffer.hpp"
#include "ethernet.hpp"
#include "protocol.hpp"
#include <iostream>

class Communicator
    : public ConditionalObserver<Buffer<Ethernet::Frame>,
                                       Protocol::Port>
{
public:
    Communicator(Protocol* channel, Protocol::Address address)
        : _channel(channel),
          _address(address),
          _semaphore(0)
    {
        _channel->attach(this, address.port);
    }

    ~Communicator() {
        _channel->detach(this, _address.port);
    }

    bool send(const Message* msg) {
        return _channel->send(
            _address,
            Protocol::Address::BROADCAST(),
            msg->data(),
            msg->size()
        ) > 0;
    }

    bool receive(Message* msg) {
        _semaphore.p();
        std::cout << "[communicator] acordou, lendo buffer\n";
        Buffer<Ethernet::Frame>* buf = _data.remove();

        if(!buf) {
            std::cout << "[communicator] buffer nulo!\n";
            msg->set_size(0);
            return false;
        }

        std::cout << "[communicator] buffer removido da fila\n";
        Protocol::Address from;
        int size = _channel->receive(buf, &from, msg->data(), Message::MAX_SIZE);
        std::cout << "[communicator] receive retornou size=" << size << "\n";
        msg->set_size(size > 0 ? size : 0);
        _channel->free(buf);
        std::cout << "[communicator] buffer liberado\n";
        return size > 0;
    }

    void update(Protocol::Port p, Buffer<Ethernet::Frame>* buf) override {
        std::cout << "[communicator] update chamado porta=" << p << "\n";
        _data.insert(buf);
        _semaphore.v();
    }

    Protocol::Port condition() const override {
        return _address.port;
    }

    const Protocol::Address& address() const {
        return _address;
    }

private:
    Protocol*         _channel;
    Protocol::Address _address;
    Semaphore         _semaphore;
    List<Buffer<Ethernet::Frame>*> _data;
};
