// observe/endpoint.hpp
#pragma once
#include "concurrent.hpp"
#include "../communicator.hpp"
#include "../message.hpp"
#include <chrono>

/*
 * Endpoint bloqueante para consumidores que NÃO são SmartData
 * (TimeClient, RSU, Gateway...). É um ConcurrentObserver que se inscreve
 * no Communicator no construtor e cancela no destrutor. Recupera o
 * receive() bloqueante, mas agora dentro da MESMA cadeia de observação:
 *
 *   NIC -> Protocol -> Communicator --notify--> Endpoint(fila) --receive--> consumidor
 *
 * Assim a pilha tem um comportamento único (push/observação); quem precisa
 * de semântica bloqueante apenas pluga um Endpoint, do mesmo jeito que o
 * SmartData se pluga como observador.
 */
class Endpoint : public ConcurrentObserver<Message, Protocol::Port> {
public:
    explicit Endpoint(Communicator& comm)
        : ConcurrentObserver<Message, Protocol::Port>(comm.address().port),
          _comm(comm)
    {
        _comm.subscribe(this);
    }

    ~Endpoint() override {
        _comm.unsubscribe(this);
    }

    // mesma semântica do antigo Communicator::receive (timeout padrão de 100ms)
    bool receive(Message* msg,
                 std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        Message* m = this->updated_for(timeout);
        if (!m) {
            msg->set_size(0);
            return false;
        }
        *msg = *m;
        delete m;                 // posse veio da fila
        return msg->size() > 0;
    }

private:
    Communicator& _comm;
};