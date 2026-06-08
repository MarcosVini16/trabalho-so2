#pragma once
#include "messages.hpp"
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
            if (!communicator.receive(&msg)) continue;
            if (msg.size() < sizeof(InterestMsg)) continue;
            InterestMsg* interest = (InterestMsg*)msg.data();
            if (interest->kind != INTEREST) continue;
            uint64_t period = interest->period;
            if (_active_periods.find(period) == _active_periods.end()) {
                _active_periods.insert(period);
                _periodic_threads.push_back(
                    std::make_unique<PeriodicThread>([this]{ send_response(); }, period)
                );
            }
        }
    }

    /*
     * Método de envio de resposta para um Interest recebido.
     * Envia uma mensagem de Response contendo os dados sensoriados pelo Transducer associado a esta unidade.
     * Esse método é chamado a uma thread
    */
    void send_response() {
        timespec ts{};
        clock_gettime(CLOCK_REALTIME, &ts);
        ResponseMsg resp;
        resp.kind = RESPONSE;
        resp.origin = Position::quadrant();
        resp.timestamp = ts.tv_sec * 1000000000ull + ts.tv_nsec;
        resp.type = UNIT;
        resp.value = (double)Transducer::sense();
        Message msg;
        std::memcpy(msg.data(), &resp, sizeof(ResponseMsg));
        msg.set_size(sizeof(ResponseMsg));
        communicator.send(&msg);
    }

private:
    Communicator communicator;
    std::unordered_set<uint64_t> _active_periods; // conjunto de períodos para os quais já existem threads ativas enviando respostas
    std::vector<std::unique_ptr<PeriodicThread>> _periodic_threads;
    Value _value;
    std::thread _thread;
    std::atomic<bool> _running{true};
};