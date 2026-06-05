#pragma once
#include "smart_data.hpp"
#include "../communicator.hpp"
#include "../protocol.hpp"
#include "../message.hpp"
#include "../utils/position.hpp"
#include <iostream>
#include <time.h>
#include <thread>
#include <atomic>

template<typename _Unit>
class InterestedSmartData : public SmartData {
public:
    static const unsigned long UNIT = _Unit::UNIT;
    // typedef typename Unit::Get<UNIT>::Type Value; (pode ser útil para acessar o tipo de dado numérico correspondente à unidade, se necessário)
public:
    InterestedSmartData(Protocol* channel, Protocol::Address addr, uint64_t period) : communicator(channel, addr), _period(period) {
        // O SmartData se registra como observador na porta associada à unidade de interesse para receber os dados
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
        timespec ts{};
        send_interest();
        while (_running) {
            Message msg;
            if (communicator.receive(&msg)) {
                if (msg.msg_type() == 1 /* Response */) {
                    clock_gettime(CLOCK_REALTIME, &ts);
                    _last_response_time = ts.tv_sec * 1000000000ull + ts.tv_nsec; // Atualiza o timestamp do último Response recebido (em nanosegundos)
                    std::cout << "Recebi mensagem para a unidade " << UNIT << " no tempo " << _last_response_time << " ns\n";
                }
            } else {
                // Timeout no receive, verificar se já passou muito tempo desde o último Response
                clock_gettime(CLOCK_REALTIME, &ts);
                uint64_t now = ts.tv_sec * 1000000000ull + ts.tv_nsec;
                if (now - _last_response_time > 2 * _period) {
                    std::cout << "No Response received for unit " << UNIT << " in the last " << _period << " ns, resending Interest\n";
                    send_interest();
                    _last_response_time = now; // Atualiza o timestamp para evitar reenvios excessivos
                }
            }
        }
    }

    void send_interest() {
        // Cria e envia uma mensagem de Interest em broadcast para solicitar os dados do Transducer associado a esta unidade
        // A mensagem deve conter o período de interesse (por exemplo, como um uint64_t no payload) para que o Transducer saiba com que frequência enviar as respostas
        // O msg_type da mensagem pode ser definido como 0 para indicar que é um Interest
        Message interest_msg;
        interest_msg.set_origin(Position::quadrant()); // Quadrante atual
        interest_msg.set_msg_type(0); // 0 = Interest
        interest_msg.set_data_type(UNIT); // Tipo de dado enviado é o Unit Code do SmartData
        std::memcpy(interest_msg.data(), &_period, sizeof(uint64_t)); // Copia o período para o payload da mensagem
        communicator.send(&interest_msg); // Envia em broadcast
    }

private:
    Communicator communicator;
    uint64_t _period; // período de interesse para este SmartData
    uint64_t _last_response_time; // timestamp do último Response recebido, para controle de timeout e reenvio de Interest
    std::thread _thread; // thread para rodar o loop de recebimento e controle de timeout
    std::atomic<bool> _running{true}; // flag para controle de execução da thread, se necessário
};