#pragma once
#include "component.hpp"
#include "../utils/ptp_frame.hpp"
#include "../engine/raw_socket_engine.hpp"
#include <time.h>
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
        : Component(address, key, mac), rs_nic(iface), _seq(0) {
            // top byte do seq = MAC byte 5 — garante que seqs de veículos diferentes nunca colidem
            _seq = static_cast<uint32_t>(mac.bytes[5]) << 24;
            protocol.attach_nic(&rs_nic);
        }

    void syncTime() {
        uint32_t seq = ++_seq;
        timespec ts;
        auto ns = [](timespec t){ return t.tv_sec * 1000000000ULL + t.tv_nsec; };

        // ====================================================================
        // Envia Sync_Req ao RSU
        // ====================================================================
        clock_gettime(CLOCK_REALTIME, &ts);
        PTPFrame req = {0, 0, seq};
        Message msg;
        std::memcpy(msg.data(), &req, sizeof(req));
        msg.set_size(sizeof(req));
        msg.set_src(communicator.address().paddr);
        send(msg);
        std::cout << "[TimeClient] Enviou Sync_Req seq=" << seq << "\n";

        // ====================================================================
        // Espera Sync do RSU (T1), mede T2 no recebimento
        // ====================================================================
        Message response;
        uint64_t t2;
        while (true) {
            if (!receive(response)) { std::cerr << "[TimeClient] Timeout Sync\n"; return; }
            if (response.size() != sizeof(PTPFrame)) continue;
            PTPFrame* r = reinterpret_cast<PTPFrame*>(response.data());
            if (r->seq == seq && r->message_type == 1) {
                clock_gettime(CLOCK_REALTIME, &ts);
                t2 = ns(ts);
                //std::cout << "[TimeClient] T2=" << t2 << "\n";
                break;
            }
        }
        uint64_t t1 = reinterpret_cast<PTPFrame*>(response.data())->timestamp;

        // ====================================================================
        // Envia Delay_Req ao RSU, mede T3 no envio
        // ====================================================================
        clock_gettime(CLOCK_REALTIME, &ts);
        uint64_t t3 = ns(ts);
        //std::cout << "[TimeClient] T3=" << t3 << "\n";
        PTPFrame dreq = {2, 0, seq};
        Message delay_req;
        std::memcpy(delay_req.data(), &dreq, sizeof(dreq));
        delay_req.set_size(sizeof(dreq));
        delay_req.set_src(communicator.address().paddr);
        send(delay_req);
        std::cout << "[TimeClient] Enviou Delay_Req seq=" << seq << "\n";

        // ====================================================================
        // Espera Delay_Resp do RSU (T4)
        // ====================================================================
        Message delay_resp;
        while (true) {
            if (!receive(delay_resp)) { std::cerr << "[TimeClient] Timeout Delay_Resp\n"; return; }
            if (delay_resp.size() != sizeof(PTPFrame)) continue;
            PTPFrame* r = reinterpret_cast<PTPFrame*>(delay_resp.data());
            if (r->seq == seq && r->message_type == 3) break;
        }
        uint64_t t4 = reinterpret_cast<PTPFrame*>(delay_resp.data())->timestamp;

        // ====================================================================
        // Calcula offset e ajusta relógio local
        // ====================================================================
        //std::cout << "[TimeClient] T1=" << t1 << " T2=" << t2 << " T3=" << t3 << " T4=" << t4 << "\n";
        //std::cout << "[TimeClient] T2-T1=" << (int64_t)(t2-t1) << " T4-T3=" << (int64_t)(t4-t3) << "\n";

        int64_t offset = ((int64_t)(t2 - t1) - (int64_t)(t4 - t3)) / 2;
        std::cout << "[TimeClient] offset=" << offset << "ns\n";

        if (!_synced) {
            clock_gettime(CLOCK_REALTIME, &ts);
            int64_t new_ns = ns(ts) + offset;
            timespec new_time = { new_ns / 1000000000LL, new_ns % 1000000000LL };
            clock_settime(CLOCK_REALTIME, &new_time);
            _synced = true;
            std::cout << "[TimeClient] Sincronizado! offset=" << offset << "ns\n";
        } else {
            std::cout << "[TimeClient] Já sincronizado, offset atual=" << offset << "ns\n";
        }
    }

    void run() {
        while (!g_stop) {
            syncTime();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

private:
    bool _synced = false;
    NIC<RawSocketEngine> rs_nic;
    uint32_t _seq;
};