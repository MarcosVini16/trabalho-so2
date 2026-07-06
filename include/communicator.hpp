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
    : public ConditionalObserver<Buffer<Ethernet::Frame>, Protocol::Port>,  // observa o Protocol
      public ConditionalObserved<Message, Protocol::Port>                   // é observado pelos SmartData
{
    using SmartDataObserved = ConditionalObserved<Message, Protocol::Port>;
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

    // SmartData se registra aqui
    void subscribe(ConditionalObserver<Message, Protocol::Port>* obs) {
        SmartDataObserved::attach(obs, _address.port);
    }
    void unsubscribe(ConditionalObserver<Message, Protocol::Port>* obs) {
        SmartDataObserved::detach(obs, _address.port);
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

    // vindo do Protocol (thread de recepção)
    void update(Protocol::Port /*p*/, Buffer<Ethernet::Frame>* buf) override {
        Message* msg = new Message();
        Protocol::Address from;

        auto* pkt = buf->data<Protocol::Packet>();
        uint8_t q = pkt->header.src_quadrant;

        int size = _channel->receive(buf, &from, msg->data(), Message::MAX_SIZE);
        msg->set_size(size > 0 ? size : 0);
        msg->set_src(from.paddr);
        msg->set_origin(q);

        // repassa (push). Se ninguém consumir, não vaza.
        if (msg->size() == 0 || !SmartDataObserved::notify(_address.port, msg))
            delete msg;
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