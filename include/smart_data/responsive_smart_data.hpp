#pragma once
#include "smart_data.hpp"
#include "../communicator.hpp"
#include "../protocol.hpp"
#include "../ethernet.hpp"
#include "../message.hpp"
#include "../utils/periodic_thread.hpp"
#include "../utils/position.hpp"
#include <iostream>
#include <unordered_set>
#include <thread>
#include <atomic>
#include <vector>

template<typename Transducer>
class ResponsiveSmartData : public SmartData {
    friend Transducer;

public:
    static const unsigned long UNIT = Transducer::UNIT;
    typedef typename Unit::Get<UNIT>::Type Value;

public:
    ResponsiveSmartData(Protocol* channel, Ethernet::Address mac):
     communicator(channel, Protocol::Address{mac, static_cast<Protocol::Port>(UNIT)}) {
        // O SmartData se registra como observador na porta associada à unidade de interesse para receber os dados
        std::cout << "Responsive Iniciado\n";
    }

    void start() override {
        _thread = std::thread(&ResponsiveSmartData::run, this);
    }

    void stop() override {
        _running = false; // Sinaliza para a thread parar
        if (_thread.joinable()) {
            _thread.join(); // Aguarda a thread terminar
        }
    }

    void run() {
        // Este método pode ser chamado para iniciar o processamento do SmartData, se necessário
        // Por exemplo, pode ser usado para configurar timers, iniciar threads, etc.
        while (_running) {
            Message msg;
            if (communicator.receive(&msg) && msg.msg_type() == 0 /* Interest */) {
                uint64_t period;
                std::cout << "Recebi interest de periodo " << (int)period << " microsegundos.\n";
                std::memcpy(&period, msg.data(), sizeof(uint64_t));
                if (_active_periods.find(period) == _active_periods.end()) {
                    _active_periods.insert(period);
                    auto task = [this]() {
                        send_response();
                    };
                    auto thread = std::make_unique<PeriodicThread>(std::move(task), period);
                    _periodic_threads.push_back(std::move(thread));
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
        communicator.send(&resp_msg);
        std::cout << "Response enviado...\n";
    }

private:
    Communicator communicator;
    std::unordered_set<uint64_t> _active_periods; // conjunto de períodos para os quais já existem threads ativas enviando respostas
    std::vector<std::unique_ptr<PeriodicThread>> _periodic_threads;
    Value _value;
    std::thread _thread;
    std::atomic<bool> _running{true};
};