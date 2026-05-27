#pragma once
#include "engine/raw_socket_engine.hpp"
#include "nic/nic.hpp"
#include "protocol.hpp"
#include "communicator.hpp"
#include "utils/ptp_frame.hpp"
#include <time.h> // Para clock_gettime()
#include <unistd.h> // Para sleep()
#include <cstdint> // Para tipos uint
#include "utils/position.hpp"

#ifdef DEBUG_PTP
    #define PTP_LOG(x) std::cout << x
#else
    #define PTP_LOG(x)
#endif

extern volatile sig_atomic_t g_stop; // variável global para sinalizar parada total, definida em vehicle_main.cpp

/*
 * Classe que representa um RSU (Road Side Unit)
 * Responsável pela sincronização temporal de todos os veículos
 * Cumpre papel de MASTER do PTP
 */
class RSU {
    public:
        RSU(const std::string& iface, uint8_t quadrant)
            : nic(iface),
              protocol(&nic),
              communicator(&protocol, Protocol::Address{nic.address(), Ports::RSU}),
              _quadrant(quadrant)
        {
            Protocol::set_quadrant(_quadrant);
            std::cout << "[RSU] quadrante=" << (int)_quadrant << "\n";
            std::cout << "[RSU] quadrante(PROTOCOL)=" << (int)Protocol::get_quadrant() << "\n";
            std::cout << "[RSU] quadrante(positionqdrnt)=" << (int)Position::quadrant() << "\n";
        }

        ~RSU() = default;

         bool send(const Message& msg) {
            return communicator.send(&msg);
        }

        bool receive(Message& msg) {
            return communicator.receive(&msg);
        }

        timespec get_time() {
            struct timespec current_time;
            clock_gettime(CLOCK_REALTIME, &current_time);
            return current_time;
        }

        Protocol::Address address() const {
            return communicator.address();
        }

        void run() {
            std::cout << "[RSU] entrando no loop\n";
            while(!g_stop) {
                std::cout << "[RSU] aguardando mensagem...\n";
                Message msg;
                // Receive bloqueia, então não há busy waiting - tirei o sleep()
                if(receive(msg)) {
                    std::cout << "[RSU] mensagem recebida size=" << msg.size() << "\n";
                    // Registra o momento de recebimento (T4 para Delay_Resp)
                    timespec receive_time;
                    clock_gettime(CLOCK_REALTIME, &receive_time);
                    PTP_LOG("[RSU] receive_time=" << (receive_time.tv_sec * 1000000000ULL + receive_time.tv_nsec) << "\n");
                    // std::cout << "[RSU] Mensagem recebida do veículo " << msg.src() << "\n";
                    if(msg.size() == sizeof(PTPFrame)) {
                        // std::cout << "[RSU] Enviando resposta de sincronização para " << msg.src() << "\n";
                        PTPFrame* ptp = static_cast<PTPFrame*>(msg.data());
                        PTPFrame response_frame;
                        Message response;
                        timespec now;
                        response_frame.seq = ptp->seq;
                        switch (ptp->message_type) {
                            case 0:
                                response_frame.message_type = 1;
                                clock_gettime(CLOCK_REALTIME, &now);
                                response_frame.timestamp = (static_cast<uint64_t>(now.tv_sec) * 1000000000) + now.tv_nsec;
                                PTP_LOG("[RSU] T1=" << response_frame.timestamp << "\n");
                                break;
                            case 2:
                                response_frame.message_type = 3;
                                response_frame.timestamp = (static_cast<uint64_t>(receive_time.tv_sec) * 1000000000) + receive_time.tv_nsec;
                                PTP_LOG("[RSU] T4=" << response_frame.timestamp << "\n");
                                break;
                            default:
                                std::cout << "[RSU] Tipo desconhecido: " << (int)ptp->message_type << "\n";
                                continue;
                        }
                        std::memcpy(response.data(), &response_frame, sizeof(response_frame));
                        response.set_size(sizeof(response_frame));
                        communicator.send_to(&response, msg.src());
                    }
                }
            }
            std::cout << "[RSU] saiu do loop g_stop=" << g_stop << "\n";
        }

    private:
        NIC<RawSocketEngine> nic; // NIC para comunicação com a rede externa
        Protocol protocol; // Protocolo de comunicação
        Communicator communicator; // Camada de comunicação para enviar/receber mensagem
        uint8_t _quadrant = 0;
};