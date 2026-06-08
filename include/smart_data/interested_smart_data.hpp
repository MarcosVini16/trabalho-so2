#pragma once
#include "messages.hpp"
#include "smart_data.hpp"
#include "../communicator.hpp"
#include "../protocol.hpp"
#include "../ethernet.hpp"
#include "../message.hpp"
#include "../utils/position.hpp"
#include <iostream>
#include <time.h>
#include <thread>
#include <atomic>

template<SmartData::Unit::Code UNIT_CODE>
class InterestedSmartData : public SmartData {
public:
    static constexpr SmartData::Unit::Code UNIT = UNIT_CODE; // Código da unidade
    // typedef typename Unit::Get<UNIT>::Type Value; (pode ser útil para acessar o tipo de dado numérico correspondente à unidade, se necessário)
public:
    InterestedSmartData(Protocol* channel, Ethernet::Address mac, uint64_t period) : _period(period),
        communicator(channel, Protocol::Address{mac, static_cast<Protocol::Port>(UNIT)})
    {
        timespec ts{};
        clock_gettime(CLOCK_REALTIME, &ts);
        _last_response_time = ts.tv_sec * 1000000000ull + ts.tv_nsec;
        std::cout << "Interested inicializado\n";
    }

    /*
     * Método para iniciar o processamento do SmartData, que pode incluir a criação de threads, timers, etc.
     * Neste caso, inicia uma thread que ficará responsável por enviar Interests periodicamente e processar as respostas recebidas.
     */
    void start() override {
        _thread = std::thread(&InterestedSmartData::run, this);
    }

    void stop() override {
        _running = false; // Sinaliza para a thread parar
        if (_thread.joinable()) {
            _thread.join(); // Aguarda a thread terminar
        }
    }

    void run() {
        send_interest();
        while (_running) {
            Message msg;
            if (!communicator.receive(&msg)) {
                // timeout - verifica se precisa reenviar
                timespec ts{};
                clock_gettime(CLOCK_REALTIME, &ts);
                uint64_t now = ts.tv_sec * 1000000000ull + ts.tv_nsec;
                if (now - _last_response_time > 2000000000ull) {
                    std::cout << "No Response received for unit " << UNIT 
                            << " in the last " << _period << " ns, resending Interest\n";
                    send_interest();
                    _last_response_time = now;
                }
                continue;
            }
            if (msg.size() < sizeof(ResponseMsg)) continue;
            ResponseMsg* resp = (ResponseMsg*)msg.data();
            if (resp->kind != RESPONSE) continue;
            _last_response_time = resp->timestamp;
            std::cout << "Recebi Response para unidade " << UNIT 
                    << " valor=" << resp->value << "\n";
        }
    }

    void send_interest() {
        timespec ts{};
        clock_gettime(CLOCK_REALTIME, &ts);
        InterestMsg interest;
        interest.kind = INTEREST;
        interest.origin = Position::quadrant();
        interest.timestamp = ts.tv_sec * 1000000000ull + ts.tv_nsec;
        interest.type = UNIT;
        interest.period = _period;
        Message msg;
        std::memcpy(msg.data(), &interest, sizeof(InterestMsg));
        msg.set_size(sizeof(InterestMsg));
        communicator.send(&msg);
    }

private:
    Communicator communicator;
    uint64_t _period; // período de interesse para este SmartData
    uint64_t _last_response_time; // timestamp do último Response recebido, para controle de timeout e reenvio de Interest
    std::thread _thread; // thread para rodar o loop de recebimento e controle de timeout
    std::atomic<bool> _running{true}; // flag para controle de execução da thread, se necessário
};