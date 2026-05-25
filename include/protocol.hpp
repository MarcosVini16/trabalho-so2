// Communication Protocol
#pragma once
#include "ethernet.hpp"
#include "nic/nic_base.hpp"
#include "utils/buffer.hpp"
#include "utils/traits.hpp"
#include "utils/ports.hpp"
#include "utils/ptp_frame.hpp"
#include "observe/conditional.hpp"
#include <arpa/inet.h> // for htons and ntohs
#include <list>
#include <execinfo.h>
#include <iostream>

class Protocol
    : public ConditionalObserver<Buffer<Ethernet::Frame>,
                                        Ethernet::Protocol>,
      public  ConditionalObserved<Buffer<Ethernet::Frame>,
                                          uint16_t>
{
    public:
        using Port             = uint16_t;
        using Physical_Address = typename NICBase::Address;
        using Buffer           = ::Buffer<Ethernet::Frame>;
        using Observer         = ConditionalObserver<Buffer, Port>;
        using Observed         = ConditionalObserved<Buffer, Port>;

        static const Ethernet::Protocol PROTO = Traits<ProtocolT>::ETHERNET_PROTOCOL_NUMBER;

        // endereço completo: MAC + porta
        struct Address {
            Physical_Address paddr;
            Port             port;

            Address() : port(0) {}
            Address(Physical_Address p, Port pt) : paddr(p), port(pt) {}

            bool operator==(const Address& o) const {
                return paddr == o.paddr && port == o.port;
            }
            operator bool() const {
                return port != 0;
            }

            static Address BROADCAST() {
                return Address(Physical_Address::BROADCAST(), 0);
            }
        };

        // header que vai na frente do payload em cada frame
        struct Header {
            uint16_t src_port;
            uint16_t dst_port;
            uint16_t payload_size; //
        } __attribute__((packed));

        static const unsigned int MTU = NICBase::MTU - sizeof(Header);

        // payload cabe em MTU bytes após o header
        struct Packet {
            Header  header;
            uint8_t data[MTU];

            Header* hdr() { return &header; }

            template<typename T>
            T* data_as() { return reinterpret_cast<T*>(data); }
        } __attribute__((packed));

    public:
        Protocol(NICBase* nic) {
            attach_nic(nic);
        }

        ~Protocol() {
            std::cout << "[protocol] destrutor iniciado\n";
            // copia a lista antes de iterar para evitar invalidar o iterador
            // auto nics_copy = _nics;
            // for(auto* nic : nics_copy) {
            //     nic->detach(this, PROTO);
            // }
            // _nics.clear();
            //std::cout << "[protocol] destrutor concluido\n";
            std::cout << "[protocol] destrutor concluido\n";
        }

        // Multiple NICs
        void attach_nic(NICBase* nic) {
            nic->attach(this, PROTO);
            _nics.push_back(nic);
        }

        void detach_nic(NICBase* nic) {
            nic->detach(this, PROTO);
            _nics.remove(nic);
        }

        /*
            * Send a message to a specific destination address and port.
            * The message is encapsulated in an Ethernet frame with the appropriate header.
            * The function iterates through all attached NICs and attempts to send the message through each one until it succeeds.
            * Returns the number of bytes sent, or -1 on error.
        */
        int send(Address src, Address dst, const void* data, unsigned int size) {
            
            // se tem peer configurado e destino é broadcast, envia para o peer
            if (_peer != Ethernet::Address() && dst.paddr == Ethernet::Address::BROADCAST())
                dst.paddr = _peer;
            
            for(auto* nic : _nics) {
                Ethernet::Address exp = nic->expected_dst();
                if (exp != Ethernet::Address() && dst.paddr != exp) {
                    continue;
                }
                //std::cout << "[protocol] NIC passou, tentando enviar...\n";
                auto* buf = nic->alloc(dst.paddr,
                                    htons(PROTO), sizeof(Header) + size); // htons to convert protocol number to network byte order (correctly filter frames in the NIC)
                if(!buf) continue;

                auto* pkt = buf->data<Packet>();

                // monta — separa o Address em seus campos primitivos
                pkt->header.src_port = htons(src.port);    // extrai a porta
                pkt->header.dst_port = htons(dst.port);
                pkt->header.payload_size = htons(static_cast<uint16_t>(size));

                std::memcpy(pkt->data, data, size);
                nic->send(buf);
                nic->free(buf);
            }
            return static_cast<int>(size);
        }

        // extrai dados de um buffer recebido e preenche src
        int receive(Buffer* buf, Address* src, void* data, unsigned int size) {
            if(!buf) return -1;
            //std::cout << "[protocol] receive chamado buf->size()=" << buf->size() << "\n";
            auto* pkt = buf->data<Packet>();
            if(src) {
                // MAC vem do frame Ethernet
                src->paddr = buf->frame()->src;
                src->port = ntohs(pkt->header.src_port);
            }
            uint16_t real_size = ntohs(pkt->header.payload_size);
            unsigned int len = std::min(size, static_cast<unsigned int>(real_size));
            std::memcpy(data, pkt->data, len);
            //std::cout << "[protocol] receive retornou len=" << len << "\n";
            return static_cast<int>(len);
        }

        // Communicators se registram aqui
        void attach(Observer* obs, Port port) {
            //std::cout << "[protocol] attach porta=" << port << "\n";
            Observed::attach(obs, port);
        }

        void free(Buffer* buf) {
            if(buf && buf->owner())
                buf->owner()->free(buf);
        }

        // seta a RSU atual
        void set_peer(Ethernet::Address peer) {
            _peer = peer;
        }

    private:
        // chamado pela NIC quando chega um frame com PROTO correto
        void update(Ethernet::Protocol, Buffer* buf) override {
            //std::cout << "[protocol] update chamado\n";
            auto* pkt = buf->data<Packet>();
            Port dst_port = ntohs(pkt->header.dst_port);
            //std::cout << "[protocol] dst_port=" << dst_port << "\n";
            // Checa se foi shm (próprio endereço)
            
            if(dst_port == 0) {
                // broadcast — verifica se é PTP
                if (ntohs(pkt->header.payload_size) == sizeof(PTPFrame)) {
                    PTPFrame* ptp = pkt->data_as<PTPFrame>();
                    if (ptp->message_type == 1 || ptp->message_type == 3) {
                        Observed::notify(Ports::TIME_CLIENT, buf);
                        free(buf);
                        return;
                    } else {
                        Observed::notify(Ports::RSU, buf);
                    }
                }
                Observed::notify(Ports::GATEWAY, buf);
            } else {
                Observed::notify(dst_port, buf);
            }

            free(buf);
        }

        // necessário para o Ordered_List filtrar por condição
        Ethernet::Protocol condition() const override {
            return PROTO;
        }

        Ethernet::Address _peer; // MAC da RSU atual, vazio = broadcast
        std::list<NICBase*> _nics; // para suportar múltiplas NICs
};