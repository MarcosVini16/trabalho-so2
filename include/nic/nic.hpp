// nic.hpp
#include "nic_base.hpp"
/*
    * A Network Interface Card (NIC) class that combines a specific Engine (E) with the NICBase interface.
    * Sends and receives Ethernet frames using the underlying Engine's capabilities, 
    * while also providing conditional observation for Protocols to register 
    * and receive notifications when frames with their EtherType are received.
*/
template<typename E>
class NIC : public NICBase,
            private E
{
public:
    template<typename... Args>
    NIC(Args&&... args) : E(std::forward<Args>(args)...) {
        _address = E::read_address();  // cada Engine sabe seu endereço
    }

    int send(Buffer* buf) override {
        return E::_send(&buf->frame, buf->size);
    }

    Buffer* alloc(Address dst, Protocol_Number prot,
                  unsigned int size) override
    {
        for(auto& buf : _buffer) {
            if(!buf.in_use) {
                buf.in_use    = true;
                buf.size      = size;
                buf.frame.dst = dst;
                buf.frame.src = _address;
                buf.frame.type = prot;
                return &buf;
            }
        }
        return nullptr;
    }

    void free(Buffer* buf) override {
        if(buf) {
            buf->in_use = false;
            buf->size   = 0;
        }
    }

    int receive(Buffer* buf, Address* src,
                void* data, unsigned int size) override
    {
        if(!buf || !buf->in_use) return -1;
        if(src) *src = buf->frame.src;
        unsigned int len = std::min(size, buf->size);
        std::memcpy(data, buf->frame.data, len);
        return static_cast<int>(len);
    }

    void attach(Observer* obs, Protocol_Number prot) override {
        Observed::attach(obs, prot);
    }

    void detach(Observer* obs, Protocol_Number prot) override {
        Observed::detach(obs, prot);
    }

    Address address() const override { return _address; }

private:
    void _handle(void* raw, size_t len) override {
        for(auto& buf : _buffer) {
            if(!buf.in_use) {
                buf.in_use = true;
                buf.size   = static_cast<unsigned int>(len);
                std::memcpy(&buf.frame, raw, len);
                this->notify(buf.frame.type, &buf);
                return;
            }
        }
        // pool esgotado — frame descartado
    }

    Address                  _address;
    Buffer<Ethernet::Frame>  _buffer[NICBase::BUFFER_COUNT];
};