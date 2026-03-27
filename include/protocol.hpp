// Communication Protocol
#include "ethernet.hpp"
#include "observe.hpp"
#include "utils/buffer.hpp"
#include "utils/traits.hpp"
template<typename NIC>
class Protocol
    : private Conditional_Data_Observer<Buffer<Ethernet::Frame>,
                                        Ethernet::Protocol>,
      public  Conditionally_Data_Observed<Buffer<Ethernet::Frame>,
                                          uint16_t>
{
    public:
        using Port             = uint16_t;
        using Physical_Address = typename NIC::Address;
        using Buffer           = ::Buffer<Ethernet::Frame>;
        using Observer         = Conditional_Data_Observer<Buffer, Port>;
        using Observed         = Conditionally_Data_Observed<Buffer, Port>;

        static const Ethernet::Protocol PROTO = Traits<Protocol>::ETHERNET_PROTOCOL_NUMBER;

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
            Address src;
            Address dst;
        } __attribute__((packed));

        static const unsigned int MTU = NIC::MTU - sizeof(Header);

        // payload cabe em MTU bytes após o header
        struct Packet {
            Header  header;
            uint8_t data[MTU];

            Header* hdr() { return &header; }

            template<typename T>
            T* data_as() { return reinterpret_cast<T*>(data); }
        } __attribute__((packed));

    public:
        Protocol(NIC* nic) : _nic(nic) {
            _nic->attach(this, PROTO);
        }

        ~Protocol() {
            _nic->detach(this, PROTO);
        }

        // envia dados de src para dst
        int send(Address src, Address dst,
                const void* data, unsigned int size)
        {
            // aloca buffer na NIC
            auto* buf = _nic->alloc(dst.paddr, PROTO, sizeof(Header) + size);
            if(!buf) return -1;

            // monta o packet no payload do frame
            auto* pkt = reinterpret_cast<Packet*>(buf->frame.data);
            pkt->header.src = src;
            pkt->header.dst = dst;
            std::memcpy(pkt->data, data, size);

            int result = _nic->send(buf);
            _nic->free(buf);
            return result;
        }

        // extrai dados de um buffer recebido e preenche src
        int receive(Buffer* buf, Address* src,
                    void* data, unsigned int size)
        {
            if(!buf) return -1;

            auto* pkt = reinterpret_cast<Packet*>(buf->frame.data);
            if(src) *src = pkt->header.src;

            unsigned int len = std::min(size, (unsigned int)MTU);
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

    private:
        // chamado pela NIC quando chega um frame com PROTO correto
        void update(Ethernet::Protocol, Buffer* buf) override {
            auto* pkt = reinterpret_cast<Packet*>(buf->frame.data);
            Port dst_port = pkt->header.dst.port;

            // repassa ao Communicator registrado nessa porta
            if(!Observed::notify(dst_port, buf))
                _nic->free(buf);   // ninguém esperando — descarta
        }

        // necessário para o Ordered_List filtrar por condição
        Ethernet::Protocol condition() const override {
            return PROTO;
        }

        NIC* _nic;
};