#pragma once
#include "observe/concurrent.hpp"
#include "observe/conditional.hpp"
#include "message.hpp"
#include "utils/buffer.hpp"
#include "ethernet.hpp"
#include "protocol.hpp"
#include <chrono>
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
        //std::cout << "Me inscrevi na port: " << address.port << "\n";
    }

    ~Communicator() {
        _channel->detach(this, _address.port);
    }

    bool send(const Message* msg) {
        //std::cout << "[communicator] send com port = " << (int)_address.port << "\n";
        return _channel->send(
            _address,
            Protocol::Address{Ethernet::Address::BROADCAST(), _address.port},
            msg->data(),
            msg->size()
        ) > 0;
    }

    // pro RSU
    bool send_to(const Message* msg, Ethernet::Address dst_mac) {
        Protocol::Address dst{dst_mac, 0};
        return _channel->send(
            _address,
            dst,
            msg->data(),
            msg->size()
        ) > 0;
    }

    bool share(const Message* msg, Protocol::Port dst_port) {
        Protocol::Address dst_addr{_address.paddr, dst_port};
        return _channel->send(
            _address,
            dst_addr,
            msg->data(),
            msg->size()
        ) > 0;
    }

    bool receive(Message* msg) {
        // bloqueia a thread até ter algum buffer na lista
        // (o update acorda via _semaphore.v())
        // _semaphore.p();
        if (!_semaphore.try_p_for(std::chrono::milliseconds(100))) { // timeout para evitar bloqueio infinito (pode ser ajustado conforme necessário)
            return false;
        }

        Message* internal = _data.empty() ? nullptr : _data.remove();

        // verifica se a mensagem é válida
        if(!internal) {
            //std::cout << "[communicator] mensagem nula!\n";
            msg->set_size(0);
            return false;
        }

        // copia os dados para o msg do caller e libera a mensagem interna
        //std::cout << "[communicator] mensagem removida da fila, size=" << internal->size() << "\n";
        *msg = *internal;
        delete internal;
        return msg->size() > 0;
    }

    // ao chegar um frame novo, update é chamado pela thread de recepção
    void update(Protocol::Port p, Buffer<Ethernet::Frame>* buf) override {
        //std::cout << "[communicator] update chamado porta=" << p << "\n";

        // copia os dados do frame para uma Message interna
        // e libera o buffer da NIC imediatamente — não fica segurando o pool
        Message* msg = new Message();
        Protocol::Address from;

        // pega o quadrante do header antes de chamar receive
        auto* pkt = buf->data<Protocol::Packet>();
        uint8_t q = pkt->header.src_quadrant;

        int size = _channel->receive(buf, &from, msg->data(), Message::MAX_SIZE);
        msg->set_size(size > 0 ? size : 0);
        msg->set_src(from.paddr);
        msg->set_origin(q);

        _data.insert(msg);
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
    List<Message*>    _data; // lista de mensagens internas (dados já copiados do buffer da NIC)
};