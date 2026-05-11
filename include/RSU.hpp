#pragma once
#include "engine/raw_socket_engine.hpp"
#include "nic/nic.hpp"
#include "protocol.hpp"
#include "communicator.hpp"
#include "utils/ptp_frame.hpp"
#include <time.h> // Para clock_gettime()
#include <unistd.h> // Para sleep()
#include <cstdint> // Para tipos uint


extern volatile sig_atomic_t g_stop; // variável global para sinalizar parada total, definida em vehicle_main.cpp

/*
 * Classe que representa um RSU (Road Side Unit)
 * Responsável pela sincronização temporal de todos os veículos
 * Cumpre papel de MASTER do PTP
 */
class RSU {
    public:
        RSU(const std::string& iface)
            : nic(iface),
              protocol(&nic),
              communicator(&protocol, Protocol::Address{nic.address(), Ports::RSU})
        {
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
            std::cout << "[RSU] Rodando...\n";
            while(!g_stop) {
                Message msg;
                // Receive bloqueia, então não há busy waiting - tirei o sleep()
                if(receive(msg)) {
                    std::cout << "[RSU] Mensagem recebida!\n";
                    // Registra o momento de recebimento (T4 para Delay_Resp)
                    timespec receive_time;
                    clock_gettime(CLOCK_REALTIME, &receive_time);

                    // std::cout << "[RSU] Mensagem recebida do veículo " << msg.src() << "\n";
                    if(msg.size() == sizeof(PTPFrame)) {
                        // std::cout << "[RSU] Enviando resposta de sincronização para " << msg.src() << "\n";
                        PTPFrame* ptp = static_cast<PTPFrame*>(msg.data());
                        PTPFrame response_frame;
                        Message response;
                        switch (ptp->message_type) {
                            case 0: // Sync_Req
                                std::cout << "[RSU] Vou responder SYNC_REQ!\n";
                                response_frame.message_type = 1; // Resposta de Sync
                                break;
                            case 2: // Delay_Req
                                std::cout << "[RSU] Vou responder DELAY_REQ!\n";
                                response_frame.message_type = 3; // Resposta de Delay_Req
                                response_frame.timestamp = (static_cast<uint64_t>(receive_time.tv_sec) * 1000000000) + receive_time.tv_nsec; // Timestamp T4 em nanosegundos
                                break;
                            default:
                                std::cout << "[RSU] Tipo de mensagem PTP desconhecido: " << (int)ptp->message_type << "\n";
                                continue; // Ignora mensagens PTP desconhecidas
                        }
                        timespec now;
                        clock_gettime(CLOCK_REALTIME, &now);
                        response_frame.timestamp = (static_cast<uint64_t>(now.tv_sec) * 1000000000) + now.tv_nsec; // Timestamp em nanosegundos
                        std::memcpy(response.data(), &response_frame, sizeof(response_frame));
                        response.set_size(sizeof(response_frame));
                        communicator.send(&response); 
                    }
                }
            }
        }

    private:
        NIC<RawSocketEngine> nic; // NIC para comunicação com a rede externa
        Protocol protocol; // Protocolo de comunicação
        Communicator communicator; // Camada de comunicação para enviar/receber mensagem
};