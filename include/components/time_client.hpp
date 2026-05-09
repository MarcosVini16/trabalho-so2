#pragma once
#include "component.hpp"
#include "../utils/ptp_frame.hpp"
#include "../engine/raw_socket_engine.hpp"
#include <time.h> // For clock_gettime
#include <unistd.h>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>

/*
 * Componente responsável por sincronizar relógio do veículo
 * Solicita a sincronização de tempo ao RSU e ajusta o relógio local
 * Cumpre papel de SLAVE do PTP
 */
class TimeClient : public Component
{
public:
    TimeClient(Protocol::Address address, key_t key, Ethernet::Address mac, const std::string& iface)
        : Component(address, key, mac), rs_nic(iface) {
            protocol.attach_nic(&rs_nic); // protocol precisa conhecer a rs_nic para enviar mensagens para o RSU
        }

    /*
     * Sincroniza o tempo do veículo com o RSU, ajustando o relógio local
     * Segue o processo de sincronização do PTP: Sync -> Delay_Req -> Delay_Resp
     */
    void syncTime() {
        // ====================================================================
        // Envio de requisição de sincronização de tempo para o RSU
        // ====================================================================
        Message msg;
        timespec current_time;
        clock_gettime(CLOCK_REALTIME, &current_time);
        uint64_t ts = current_time.tv_sec * 1000000000ULL + current_time.tv_nsec; // Convert to nanoseconds
        PTPFrame ptp_frame = {0,  0 }; // message_type = 0 (Req), timestamp = 0 (não usado para Req)
        std::memcpy(msg.data(), &ptp_frame, sizeof(PTPFrame));
        msg.set_size(sizeof(PTPFrame));
        msg.set_timestamp(ts); // timestamp = 0 (não usado para Req)
        msg.set_src(communicator.address().paddr); // Define o endereço de origem da mensagem
        send(msg);
        std::cout << "[TimeClient] Enviou requisição de sincronização de tempo com timestamp " << ts << "\n";

        // ====================================================================
        // Espera pelo SYNC do RSU (T1) e registra o momento de recebimento (T2)
        // ====================================================================
        Message response;
        bool received = receive(response);
        clock_gettime(CLOCK_REALTIME, &current_time); // T2 = momento de recebimento do SYNC do RSU
        // Converte current_time para nanosegundos para facilitar o cálculo do offset
        uint64_t t2 = current_time.tv_sec * 1000000000ULL + current_time.tv_nsec;

        if(!received) {
            std::cerr << "[TimeClient] Erro ao receber resposta do RSU\n";
            return;
        }
        if (response.size() != sizeof(PTPFrame)) {
            std::cerr << "[TimeClient] Resposta do RSU tem tamanho inesperado: " << response.size() << "\n";
            return;
        }
        PTPFrame* resp_frame = reinterpret_cast<PTPFrame*>(response.data());
        if (resp_frame->message_type != 1) { // message_type = 1 (Resp)
            std::cerr << "[TimeClient] Resposta do RSU tem tipo inesperado: " << static_cast<int>(resp_frame->message_type) << "\n";
            return;
        }
        uint64_t t1 = resp_frame->timestamp; // T1 = momento de envio do SYNC pelo RSU (timestamp da MSG)
        
        // ====================================================================
        // Envia DELAY_REQ para o RSU (T3)
        // ====================================================================
        Message delay_req;
        PTPFrame delay_req_frame = {2, 0}; // message_type = 2 (Delay_Req), length = sizeof(PTPFrame), timestamp = 0 (não usado para Delay_Req)
        std::memcpy(delay_req.data(), &delay_req_frame, sizeof(delay_req_frame));
        delay_req.set_size(sizeof(delay_req_frame));
        delay_req.set_src(communicator.address().paddr); // Define endereço
        clock_gettime(CLOCK_REALTIME, &current_time);
        uint64_t t3 = current_time.tv_sec * 1000000000ULL + current_time.tv_nsec; // Timestamp T3 local
        delay_req.set_timestamp(t3);
        send(delay_req);
        std::cout << "[TimeClient] Enviou requisição de delay com timestamp " << t3 << "\n";

        // ====================================================================
        // Espera pelo DELAY_RESP do RSU (T4)
        // ====================================================================

        Message delay_resp;
        received = receive(delay_resp);
        
        if (!received) {
            std::cerr << "[TimeClient] Erro ao receber resposta de delay do RSU\n";
            return;
        }
        if (delay_resp.size() != sizeof(PTPFrame)) {
            std::cerr << "[TimeClient] Resposta de delay do RSU tem tamanho inesperado: " << delay_resp.size() << "\n";
            return;
        }
        PTPFrame* delay_resp_frame = reinterpret_cast<PTPFrame*>(delay_resp.data());
        if (delay_resp_frame->message_type != 3) { // message_type = 3 (Delay_Resp)
            std::cerr << "[TimeClient] Resposta de delay do RSU tem tipo inesperado: " << static_cast<int>(delay_resp_frame->message_type) << "\n";
            return;
        }
        uint64_t t4 = delay_resp_frame->timestamp; // T4 = momento de recebimento do DELAY_REQ pelo RSU (timestamp da MSG)

        // ====================================================================
        // Calcula delay e offset e ajusta o relógio local
        // ====================================================================
        // Cálculo do delay: delay = ((T2 - T1) + (T4 - T3)) / 2
        int64_t delay = ((int64_t)(t2 - t1) + (int64_t)(t4 - t3)) / 2;
        // Cálculo do offset: offset = ((T2 - T1) - (T4 - T3)) / 2
        int64_t offset = ((int64_t)(t2 - t1) - (int64_t)(t4 - t3)) / 2;
        std::cout << "[TimeClient] Cálculo de delay: " << delay << " ns, offset: " << offset << " ns\n";
        // Ajuste do relógio local: new_time = current_time + offset
        timespec new_time;
        clock_gettime(CLOCK_REALTIME, &current_time);
        int64_t new_time_ns = (current_time.tv_sec * 1000000000ULL + current_time.tv_nsec) + offset;
        new_time.tv_sec = new_time_ns / 1000000000ULL;
        new_time.tv_nsec = new_time_ns % 1000000000ULL;
        // Seta o novo clock da VM (requer privilégios de root)
        clock_settime(CLOCK_REALTIME, &new_time);
        std::cout << "[TimeClient] Relógio local ajustado, novo tempo: " << new_time.tv_sec << "s " << new_time.tv_nsec << "ns\n";
    }

    key_t getClockKey() const {
        return clock_key;
    }

    /*
     * Executa o loop principal do TimeClient, 
     * sincronizando o tempo periodicamente com o RSU
     */
    void run() {
        while (true) {
            syncTime();
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Sincroniza a cada 500ms (ajustável conforme necessário)
        }
    }

private:
    NIC<RawSocketEngine> rs_nic; // NIC usando Raw Socket para comunicação direta com o RSU
};