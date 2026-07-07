#pragma once
#include "observe/conditional.hpp"
#include "message.hpp"
#include "utils/buffer.hpp"
#include "ethernet.hpp"
#include "protocol.hpp"
#include <iostream>

/*
 * Communicator: nó puro da cadeia de observação, com UM comportamento só.
 *  - Observa o Protocol (recebe frames via update()).
 *  - É observado pelos consumidores (SmartData e Endpoints) via notify().
 *
 * Não tem mais fila/semáforo próprios: quem precisa de receive() bloqueante
 * pluga um Endpoint (que é um ConcurrentObserver). update() só monta a
 * Message e repassa; se ninguém está inscrito, descarta.
 */
class Communicator
    : public ConditionalObserver<Buffer<Ethernet::Frame>, Protocol::Port>,
      public ConditionalObserved<Message, Protocol::Port>
{
    using Observed = ConditionalObserved<Message, Protocol::Port>;
public:
    Communicator(Protocol* channel, Protocol::Address address)
        : _channel(channel),
          _address(address)
    {
        _channel->attach(this, address.port);
    }

    ~Communicator() {
        _channel->detach(this, _address.port);
    }

    bool send(const Message* msg) {
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

    // Consumidores (SmartData, Endpoint) se inscrevem/desinscrevem aqui
    void subscribe(ConditionalObserver<Message, Protocol::Port>* obs) {
        Observed::attach(obs, _address.port);
    }
    void unsubscribe(ConditionalObserver<Message, Protocol::Port>* obs) {
        Observed::detach(obs, _address.port);
    }

    // ao chegar um frame novo, update é chamado pela thread de recepção
    void update(Protocol::Port /*p*/, Buffer<Ethernet::Frame>* buf) override {
        // copia os dados do frame para uma Message interna e libera o buffer
        // da NIC imediatamente — não fica segurando o pool
        Message* msg = new Message();
        Protocol::Address from;

        // pega o quadrante do header antes de chamar receive
        auto* pkt = buf->data<Protocol::Packet>();
        uint8_t q = pkt->header.src_quadrant;

        int size = _channel->receive(buf, &from, msg->data(), Message::MAX_SIZE);
        msg->set_size(size > 0 ? size : 0);
        msg->set_src(from.paddr);
        msg->set_origin(q);

        // push: a posse vai pro observador que consumir.
        // Se ninguém está inscrito (notify == false), descarta.
        if (msg->size() == 0 || !Observed::notify(_address.port, msg)) {
            delete msg;
        }
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
};