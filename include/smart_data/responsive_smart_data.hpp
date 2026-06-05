#include "smart_data.hpp"
#include "../communicator.hpp"
#include "../observe/conditional.hpp"
#include "../message.hpp"
#include "../utils/periodic_thread.hpp"
#include "../utils/position.hpp"
#include <iostream>
#include <unordered_set>

template<typename Transducer>
class ResponsiveSmartData : public SmartData {
    friend Transducer;

public:
    static const unsigned long UNIT = Transducer::UNIT;
    typedef typename Unit::Get<UNIT>::Type Value;

public:
    ResponsiveSmartData(Communicator* comm) : _comm(comm), _port(Transducer::PORT) {
        // O Transducer deve definir uma porta específica para este SmartData, que será usada para receber os dados
        // Depois, o SmartData se registra como observador nessa porta para receber os dados do Transducer
        attach_comm(comm, _port);
    }

    void run() {
        // Este método pode ser chamado para iniciar o processamento do SmartData, se necessário
        // Por exemplo, pode ser usado para configurar timers, iniciar threads, etc.
        while (true) {
            Message msg;
            if (_comm->receive(&msg) && msg.msg_type == 0 /* Interest */) {
                uint64_t period;
                std::memcpy(&period, msg.data(), sizeof(uint64_t));
                if (_active_periods.find(period) == _active_periods.end()) {
                    _active_periods.insert(period);
                    auto task = [this]() {
                        send_response();
                    };
                    auto thread = std::make_unique<PeriodicThread>(std::move(task), period);
                }
            }
        }
    }

    /*
     * Método de envio de resposta para um Interest recebido.
     * Envia uma mensagem de Response contendo os dados sensoriados pelo Transducer associado a esta unidade.
     * Esse método é chamado a uma thread
    */
    void send_response() {
        _value = Transducer::sense(); // Obtém o valor sensoriado do Transducer (a implementação específica dependerá de como o Transducer é definido)
        Message resp_msg;
        resp_msg.set_origin(Position::quadrant()); // Define a origem da mensagem como o quadrante atual (pode ser útil para o receptor identificar de onde veio a resposta)
        resp_msg.set_msg_type(1); // Response
        resp_msg.set_data_type(UNIT);
        std::memcpy(resp_msg.data(), &_value, sizeof(Value));
        resp_msg.set_size(sizeof(Value));
        _comm->send(resp_msg);
    }

    void attach_comm(Communicator* comm, Protocol::Port port) {
        // port deve ser a porta associada a este SmartData (definida pelo Transducer)
        _comm->attach(this, port);
    }

private:
    Communicator* _comm;
    Protocol::Port _port; // porta associada a este SmartData, definida pelo Transducer
    std::unordered_set<uint64_t> _active_periods; // conjunto de períodos para os quais já existem threads ativas enviando respostas
    Value _value; //
};