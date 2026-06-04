#include "smart_data.hpp"
#include "../communicator.hpp"
#include "../observe/conditional.hpp"
#include "../message.hpp"
#include "../utils/position.hpp"
#include <iostream>
#include <time.h>

template<typename _Unit>
class InterestedSmartData : public SmartData {
public:
    static const unsigned long UNIT = _Unit::UNIT;
    // typedef typename Unit::Get<UNIT>::Type Value; (pode ser útil para acessar o tipo de dado numérico correspondente à unidade, se necessário)
public:
    InterestedSmartData(Communicator* comm, uint64_t period) : _comm(comm), _period(period) {
        // O SmartData se registra como observador na porta associada à unidade de interesse para receber os dados
        attach_comm(comm, _Unit::PORT);
    }

    void attach_comm(Communicator* comm, Protocol::Port port) {
        // port deve ser a porta associada a este SmartData (definida pelo Unit)
        comm->attach(this, port);
    }

    void run() {
        // Este método pode ser chamado para iniciar o processamento do SmartData, se necessário
        // Por exemplo, pode ser usado para configurar timers, iniciar threads, etc.
        // Pseudocódigo:
        // Primeiramente, envia Interesse inicial em broadcast
        // Aí, em while true, fica esperando por Responses
        // Se ficar um tempo sem receber Response, pode reenviar o Interest
        // O receive do Communicator naturalmente desbloqueia depois de muito tempo travado
        timespec ts{};
        send_interest();
        while (true) {
            Message msg;
            if (_comm->receive(&msg)) {
                if (msg.msg_type() == 1 /* Response */) {
                    clock_gettime(CLOCK_REALTIME, &ts);
                    _last_response_time = ts.tv_sec * 1000000000ull + ts.tv_nsec;
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
        _comm->send(&interest_msg); // Envia em broadcast
    }

    void update(Protocol::Port p, Buffer<Ethernet::Frame>* buf) override {
        // Processa o buffer recebido e atualiza os dados conforme necessário
        // (a implementação específica dependerá do formato dos dados recebidos e do que o SmartData precisa fazer com eles)
        // Exemplo: extrair o valor do payload, converter para o tipo numérico correto, etc.

        // Depois de processar, libera o buffer
        free(buf);
    }
private:
    Communicator* _comm;
    uint64_t _period; // período de interesse para este SmartData
    uint64_t _last_response_time; // timestamp do último Response recebido, para controle de timeout e reenvio de Interest
};