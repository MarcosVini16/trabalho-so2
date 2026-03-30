// Communication Protocol
#pragma once
#include "ethernet.hpp"
#include "nic/nic_base.hpp"
#include "utils/buffer.hpp"
#include "utils/traits.hpp"
#include "observe/conditional.hpp"
#include <arpa/inet.h> // for htons and ntohs
#include <list>

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
            Ethernet::Address src;
            Ethernet::Address dst;
            uint16_t src_port;
            uint16_t dst_port;
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
            for(auto* nic : _nics)
                detach_nic(nic);
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
            for(auto* nic : _nics) {
                auto* buf = nic->alloc(Ethernet::Address::BROADCAST(),
                                    htons(PROTO), sizeof(Header) + size); // htons to convert protocol number to network byte order (correctly filter frames in the NIC)
                if(!buf) continue;

                auto* pkt = buf->data<Packet>();

                // monta — separa o Address em seus campos primitivos
                pkt->header.src  = src.paddr;   // extrai o MAC
                pkt->header.src_port = src.port;    // extrai a porta
                pkt->header.dst  = dst.paddr;
                pkt->header.dst_port = dst.port;

                std::memcpy(pkt->data, data, size);
                nic->send(buf);
                nic->free(buf);
            }
            return static_cast<int>(size);
        }

        // extrai dados de um buffer recebido e preenche src
        int receive(Buffer* buf, Address* src, void* data, unsigned int size) {
            if(!buf) return -1;
            auto* pkt = buf->data<Packet>();

            // desmonta — reconstrói o Address a partir dos campos primitivos
            if(src) {
                src->paddr = pkt->header.src;   // junta de volta
                src->port  = pkt->header.src_port;
            }

            unsigned int len = std::min(size, static_cast<unsigned int>(MTU));
            std::memcpy(data, pkt->data, len);
            return static_cast<int>(len);
        }

        // Communicators se registram aqui
        void attach(Observer* obs, Port port) {
            Observed::attach(obs, port);
        }
        void detach(Observer* obs, Port port) {
            Observed::detach(obs, port);
        }

        void free(Buffer* buf) {
            if(buf && buf->owner())
                buf->owner()->free(buf);
        }

    private:
        // chamado pela NIC quando chega um frame com PROTO correto
        void update(Ethernet::Protocol, Buffer* buf) override {
            auto* pkt = buf->data<Packet>();

            // desmonta só a porta destino — é tudo que precisa para o notify
            Port dst_port = pkt->header.dst_port;

            if(!Observed::notify(dst_port, buf))
                free(buf);
        }

        // necessário para o Ordered_List filtrar por condição
        Ethernet::Protocol condition() const override {
            return PROTO;
        }

        std::list<NICBase*> _nics; // para suportar múltiplas NICs
};