#include "smart_data.hpp"
#include "../communicator.hpp"
#include "../observe/conditional.hpp"

template<typename Transducer>
class ResponsiveSmartData : public SmartData, ConditionalObserver<Buffer<Ethernet::Frame>, Protocol::Port> {
    friend Transducer;

public:
    static const unsigned long UNIT = Transducer::UNIT;
    // typedef typename Unit::Get<UNIT>::Type Value; (pode ser útil para acessar o tipo de dado numérico correspondente à unidade, se necessário)

public:
    ResponsiveSmartData(Communicator* comm) : _comm(comm) {
        // O Transducer deve definir uma porta específica para este SmartData, que será usada para receber os dados
        // Exemplo: _port = Transducer::PORT;
        // Depois, o SmartData se registra como observador nessa porta para receber os dados do Transducer
        // attach_comm(comm, _port);
    }

    void attach_comm(Communicator* comm, Protocol::Port port) {
        // port deve ser a porta associada a este SmartData (definida pelo Transducer)
        _comm->attach(this, port);
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
    Protocol::Port _port; // porta associada a este SmartData, definida pelo Transducer
};