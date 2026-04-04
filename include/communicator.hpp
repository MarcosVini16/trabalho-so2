#pragma once
#include "observe/concurrent.hpp"
#include "observe/conditional.hpp"
#include "message.hpp"
#include "utils/buffer.hpp"
#include "ethernet.hpp"
#include "protocol.hpp"
#include <iostream>
#include <mutex>

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
        // bloqueia a thread até ter algum buffer na lista
        // (o update acorda via _semaphore.v())
        _semaphore.p();
        std::cout << "[communicator] acordou, lendo buffer\n";
        Buffer<Ethernet::Frame>* buf;
        
        // quando acorda: trava o mutex antes de fazer o remove na lista
        // remove o buffer da lista
        // libera o mutex
        {
            std::lock_guard<std::mutex> lock(_mutex);
            buf = _data.remove();
        }

        // verifica se o buffer é válido
        if(!buf) {
            std::cout << "[communicator] buffer nulo!\n";
            msg->set_size(0);
            return false;
        }

        // continua processando o buffer com segurança
        std::cout << "[communicator] buffer removido da fila\n";
        Protocol::Address from;
        int size = _channel->receive(buf, &from, msg->data(), Message::MAX_SIZE);
        std::cout << "[communicator] receive retornou size=" << size << "\n";
        msg->set_size(size > 0 ? size : 0);
        _channel->free(buf);
        std::cout << "[communicator] buffer liberado\n";
        return size > 0;
    }

    // ao chegar um frame novo, update é chamado pela thread de recepção
    void update(Protocol::Port p, Buffer<Ethernet::Frame>* buf) override {
        std::cout << "[communicator] update chamado porta=" << p << "\n";
        {   
            // antes de inserir o buffer na lista, trava o mutex pra garantir que nenhuma outra thread está mexendo na lista ao mesmo tempo
            std::lock_guard<std::mutex> lock(_mutex);
            // insere o buffer na lista
            _data.insert(buf);
        }
        // acorda a thread que está bloqueada no receive
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
    std::mutex        _mutex;
    List<Buffer<Ethernet::Frame>*> _data;
};
