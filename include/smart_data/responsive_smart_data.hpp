#include "smart_data.hpp"
#include "../communicator.hpp"
#include "../observe/conditional.hpp"
#include "../message.hpp"
#include <unordered_set>

template<typename Transducer>
class ResponsiveSmartData : public SmartData {
    friend Transducer;

public:
    static const unsigned long UNIT = Transducer::UNIT;
    // typedef typename Unit::Get<UNIT>::Type Value; (pode ser útil para acessar o tipo de dado numérico correspondente à unidade, se necessário)

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
                    // cria thread periódica que envia Response a cada 'period' nanosegundos
                    // (a implementação específica dependerá de como você deseja gerenciar as threads e os períodos)
                }
            }
        }
    }

    void attach_comm(Communicator* comm, Protocol::Port port) {
        // port deve ser a porta associada a este SmartData (definida pelo Transducer)
        _comm->attach(this, port);
    }
    
private:
    Communicator* _comm;
    Protocol::Port _port; // porta associada a este SmartData, definida pelo Transducer
    std::unordered_set<uint64_t> _active_periods; // conjunto de períodos para os quais já existem threads ativas enviando respostas
    // {2000, 3000}
};