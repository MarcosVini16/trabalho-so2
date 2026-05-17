#pragma once

#include "component.hpp"
#include "../utils/ptp_frame.hpp"
#include "../engine/raw_socket_engine.hpp"
#include "../utils/stats.hpp"

#include <time.h>
#include <unistd.h>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>

#ifdef DEBUG_PTP
    #define PTP_LOG(x) std::cout << x
#else
    #define PTP_LOG(x)
#endif

/*
 * Componente responsável por sincronizar relógio do veículo
 * Atua como SLAVE do protocolo PTP simplificado
 */
class TimeClient : public Component
{
public:
    TimeClient(
        Protocol::Address address,
        key_t key,
        Ethernet::Address mac,
        const std::string& iface
    )
        : Component(address, key, mac),
          rs_nic(iface),
          _seq(0)
          // a semente do rng é o último byte do MAC
          //_rng(static_cast<uint32_t>(mac.bytes[5]) * 12345)
    {
        // byte alto do seq = último byte do MAC
        // evita colisão entre veículos diferentes
        // (seq base serve pra garantir que _seq não vai ¨invadir¨ o endereço MAC, isso é garantido usando uma máscara)
        // basicamente (preserva o endereço MAC)
        _seq_base = static_cast<uint32_t>(mac.bytes[5]) << 24;
        _seq = _seq_base;

        protocol.attach_nic(&rs_nic);
    }

    void syncTime() 
    {

        // avança seq e garante que não vai invadir o endereço MAC
        _seq = _seq_base | ((_seq + 1) & 0x00FFFFFF);

        timespec ts{};

        auto ns = [](const timespec& t) -> int64_t
        {
            return (int64_t)t.tv_sec * 1000000000LL + t.tv_nsec;
        };

        // ============================================================
        // T1 -> envio Sync_Req
        // ============================================================

        clock_gettime(CLOCK_REALTIME, &ts);

        PTPFrame req{};
        req.message_type = 0;
        req.timestamp    = 0;
        req.seq          = _seq;

        Message msg;

        std::memcpy(msg.data(), &req, sizeof(req));

        msg.set_size(sizeof(req));
        msg.set_src(communicator.address().paddr);

        send(msg);
        
        PTP_LOG("[TimeClient] Enviou Sync_Req seq=" << _seq << "\n");

        // ============================================================
        // Espera Sync_Resp
        // T1 vem do RSU
        // T2 é medido localmente no recebimento
        // ============================================================

        Message response{};

        int64_t t1 = 0;
        int64_t t2 = 0;

        while (true)
        {
            if (!receive(response))
            {
                std::cerr
                    << "[TimeClient] Timeout Sync\n";

                g_stats.ptp_sync_timeout++;
                return;
            }

            if (response.size() != sizeof(PTPFrame))
                continue;

            PTPFrame* r =
                reinterpret_cast<PTPFrame*>(response.data());

            if (r->seq == _seq && r->message_type == 1)
            {
                clock_gettime(CLOCK_REALTIME, &ts);

                t2 = ns(ts);
                t1 = (int64_t)r->timestamp;

                break;
            }
        }

        // ============================================================
        // T3 -> envio Delay_Req
        // ============================================================

        clock_gettime(CLOCK_REALTIME, &ts);

        int64_t t3 = ns(ts);

        PTPFrame dreq{};
        dreq.message_type = 2;
        dreq.timestamp    = 0;
        dreq.seq          = _seq;

        Message delay_req{};

        std::memcpy(delay_req.data(), &dreq, sizeof(dreq));

        delay_req.set_size(sizeof(dreq));
        delay_req.set_src(communicator.address().paddr);

        send(delay_req);

        PTP_LOG("[TimeClient] Enviou Delay_Req seq=" << _seq << "\n");
        // ============================================================
        // Espera Delay_Resp
        // T4 vem do RSU
        // ============================================================

        Message delay_resp{};

        int64_t t4 = 0;

        while (true)
        {
            if (!receive(delay_resp))
            {
                std::cerr
                    << "[TimeClient] Timeout Delay_Resp\n";

                g_stats.ptp_sync_timeout++;
                return;
            }

            if (delay_resp.size() != sizeof(PTPFrame))
                continue;

            PTPFrame* r =
                reinterpret_cast<PTPFrame*>(delay_resp.data());

            if (r->seq == _seq && r->message_type == 3)
            {
                t4 = (int64_t)r->timestamp;
                break;
            }
        }

        // ============================================================
        // Offset PTP
        //
        // offset =
        // ((T2 - T1) + (T3 - T4)) / 2
        // ============================================================

        int64_t offset =
            ((t2 - t1) + (t3 - t4)) / 2;

        PTP_LOG("[TimeClient] T1=" << t1 << " T2=" << t2 << " T3=" << t3 << " T4=" << t4 << "\n");
        PTP_LOG("[TimeClient] offset=" << offset << "ns\n");

        g_stats.last_offset_ns = offset;
        g_stats.ptp_sync_ok++;

        // ============================================================
        // PLL simples
        //
        // aplica apenas fração do erro
        // ============================================================

        clock_gettime(CLOCK_REALTIME, &ts);

        int64_t now_ns = ns(ts);

        // correction deve ter sinal oposto ao offset
        // não é aplicado integralmente pois é instável,
        // melhor uma abordagem mais conservadora
        // (testes demonstraram melhor desempenho)
        int64_t correction = -offset;

        int64_t new_ns = now_ns + correction;

        timespec new_time{};

        new_time.tv_sec  = new_ns / 1000000000LL;
        new_time.tv_nsec = new_ns % 1000000000LL;

        // normalização
        if (new_time.tv_nsec < 0)
        {
            new_time.tv_nsec += 1000000000LL;
            new_time.tv_sec--;
        }

        // ajusta relógio do sistema
        if (clock_settime(CLOCK_REALTIME, &new_time) != 0)
        {
            perror("[TimeClient] clock_settime");
        }
        
        PTP_LOG("[TimeClient] correction=" << correction << "\n");
    }

    void run()
    {   
        // o último offset é guardado para o algoritmo de intervalo adaptativo
        int64_t last_offset = 0;
        // começa em 500ms
        int interval_ms = 500;


        while (!g_stop)
        {
            syncTime();
            
            // calcula a variação do offset
            int64_t delta = std::abs(g_stats.last_offset_ns - last_offset);
            last_offset = g_stats.last_offset_ns;

            // calcula o intervalo baseado na volatilidade
            if (delta < 1000000LL) {
                interval_ms = std::min(interval_ms * 2, 1000);
            } else {
                interval_ms = std::max(interval_ms / 2, 500);
            }
            // aplica
            std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));

        }

        PTP_LOG("\n====== PTP STATS ======\n");
        PTP_LOG("PTP sync OK: " << g_stats.ptp_sync_ok << "\n");
        PTP_LOG("PTP timeouts: " << g_stats.ptp_sync_timeout << "\n");
        PTP_LOG("Ultimo offset: " << g_stats.last_offset_ns << " ns\n");
        PTP_LOG("=======================\n");
    }

private:
    NIC<RawSocketEngine> rs_nic;

    uint32_t _seq;
    uint32_t _seq_base;

    bool _synced = false;
};