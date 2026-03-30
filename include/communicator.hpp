// Communication End-Point (for client classes)
#pragma once
#include "observe/concurrent.hpp"
#include "observe/conditional.hpp"
#include "message.hpp"
#include "utils/buffer.hpp"
#include "ethernet.hpp"
#include "protocol.hpp"

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
        _semaphore.p(); // bloqueia
        std::cout << "[communicator] mensagem chegou!\n";
        Buffer<Ethernet::Frame> buf = _data.remove();    // retira da fila

        Protocol::Address from;
        int size = _channel->receive(&buf, &from,
                                     msg->data(), msg->size());
        msg->set_size(size);
        _channel->free(&buf);
        return size > 0;
    }

    // chamado pelo Protocol quando chega um frame na nossa porta
    void update(Protocol::Port, Buffer<Ethernet::Frame>* buf) override {
        _data.insert(*buf);   // insere na fila
        _semaphore.v();      // acorda o receive()
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
    List<Buffer<Ethernet::Frame>> _data;
};