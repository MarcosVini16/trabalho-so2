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

#include <random>

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
        _seq = static_cast<uint32_t>(mac.bytes[5]) << 24;

        protocol.attach_nic(&rs_nic);
    }

    void syncTime() 
    {
        uint32_t seq = ++_seq;

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
        req.seq          = seq;

        Message msg;

        std::memcpy(msg.data(), &req, sizeof(req));

        msg.set_size(sizeof(req));
        msg.set_src(communicator.address().paddr);

        send(msg);

        std::cout
            << "[TimeClient] Enviou Sync_Req seq="
            << seq
            << "\n";

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

            if (r->seq == seq && r->message_type == 1)
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
        dreq.seq          = seq;

        Message delay_req{};

        std::memcpy(delay_req.data(), &dreq, sizeof(dreq));

        delay_req.set_size(sizeof(dreq));
        delay_req.set_src(communicator.address().paddr);

        send(delay_req);

        std::cout
            << "[TimeClient] Enviou Delay_Req seq="
            << seq
            << "\n";

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

            if (r->seq == seq && r->message_type == 3)
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

        std::cout
            << "[TimeClient] "
            << "T1=" << t1
            << " T2=" << t2
            << " T3=" << t3
            << " T4=" << t4
            << "\n";

        std::cout
            << "[TimeClient] offset="
            << offset
            << "ns\n";
        g_stats.last_offset_ns = offset;
        g_stats.ptp_sync_ok++;

        // ============================================================
        // PLL simples
        //
        // aplica apenas fração do erro
        // ============================================================

        clock_gettime(CLOCK_REALTIME, &ts);

        int64_t now_ns = ns(ts);

        // IMPORTANTE:
        // correction deve ter sinal oposto ao offset
        int64_t correction = -offset / 8;

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

        std::cout
            << "[TimeClient] correction="
            << correction
            << "\n";
    }

    void run()
    {   
        // jitter inicial - desincroniza os veículos no boot
        // um tempo diferente a cada ciclo, escolhido aleatóriamente
        //std::uniform_int_distribution<int> init_dist(0, 499);

        //std::this_thread::sleep_for(std::chrono::milliseconds(init_dist(_rng)));
        while (!g_stop)
        {
            syncTime();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << "\n====== PTP STATS ======\n";

        std::cout
            << "PTP sync OK: "
            << g_stats.ptp_sync_ok
            << "\n";

        std::cout
            << "PTP timeouts: "
            << g_stats.ptp_sync_timeout
            << "\n";

        std::cout
            << "Ultimo offset: "
            << g_stats.last_offset_ns
            << " ns\n";

        std::cout << "=======================\n";
    }

private:
    NIC<RawSocketEngine> rs_nic;

    uint32_t _seq;

    // gerador de números aleatórios
    //std::mt19937 _rng;

    bool _synced = false;
};