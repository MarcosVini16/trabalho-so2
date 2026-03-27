// Network
#include "utils/traits.hpp"
#include "utils/buffer.hpp"
#include "engine/engine.hpp"
#include "ethernet.hpp"

template<typename E>
class NIC : public Ethernet,
            public Conditionally_Data_Observed
                Buffer<Ethernet::Frame>,
                Ethernet::Protocol>,
            private E
{
    public:
        using Address         = Ethernet::Address;
        using Protocol_Number = Ethernet::Protocol;
        using Observer        = Conditional_Data_Observer
                                    Buffer<Ethernet::Frame>,
                                    Ethernet::Protocol>;
        using Observed        = Conditionally_Data_Observed
                                    Buffer<Ethernet::Frame>,
                                    Ethernet::Protocol>;

        static const unsigned int SEND_BUFFERS    = Traits<NIC>::SEND_BUFFERS;
        static const unsigned int RECEIVE_BUFFERS = Traits<NIC>::RECEIVE_BUFFERS;
        static const unsigned int BUFFER_COUNT    = SEND_BUFFERS + RECEIVE_BUFFERS;

    public:
        // Forward constructor arguments to the Engine base class (E)
        template<typename... Args>
        NIC(Args&&... args) : E(std::forward<Args>(args)...) {}

        // Return the MAC address of this NIC
        Address address() const { return _address; }

        // Allocate a buffer for sending data. The caller should fill the buffer's frame.data with the payload and then call send() to transmit it.
        Buffer<Ethernet::Frame>* alloc(Address dst,
                                    Protocol_Number prot,
                                    unsigned int size)
        {
            for(auto& buf : _buffer) {
                if(!buf.in_use) {
                    buf.in_use      = true;
                    buf.size        = sizeof(Ethernet::Frame::dst)
                                    + sizeof(Ethernet::Frame::src)
                                    + sizeof(Ethernet::Frame::type)
                                    + size;
                    buf.frame.dst   = dst;
                    buf.frame.src   = _address;
                    buf.frame.type  = prot;
                    return &buf;
                }
            }
            return nullptr;  // pool esgotado
        }

        // Send a buffer that has been allocated and filled with data. The buffer will be marked as free after sending.
        int send(Buffer<Ethernet::Frame>* buf) {
            return E::_send(&buf->frame, buf->size);
        }

        // Convenience function to send data without manually allocating a buffer. This function will allocate a buffer, copy the data into it, send it, and then free the buffer.
        int send(Address dst, Protocol_Number prot,
                const void* data, unsigned int size)
        {
            auto* buf = alloc(dst, prot, size);
            if(!buf) return -1;
            std::memcpy(buf->frame.data, data, size);
            int result = send(buf);
            free(buf);
            return result;
        }

        // Receive data into the provided buffer. The source address will be stored in src if it's not null. The function returns the number of bytes received, or -1 on error.
        int receive(Buffer<Ethernet::Frame>* buf,
                    Address* src,
                    void* data,
                    unsigned int size)
        {
            if(!buf || !buf->in_use) return -1;
            if(src) *src = buf->frame.src;
            unsigned int len = std::min(size, buf->size);
            std::memcpy(data, buf->frame.data, len);
            return static_cast<int>(len);
        }

        // Free a buffer that was previously allocated. This marks the buffer as available for future use.
        void free(Buffer<Ethernet::Frame>* buf) {
            if(buf) {
                buf->in_use = false;
                buf->size   = 0;
            }
        }

        // attach and detach observers for specific protocol numbers. Observers will be notified when a frame with the corresponding EtherType is received.
        void attach(Observer* obs, Protocol_Number prot) {
            Observed::attach(obs, prot);
        }
        void detach(Observer* obs, Protocol_Number prot) {
            Observed::detach(obs, prot);
        }

    private:
        // This function is called by the Engine base class (E) when a new Ethernet frame is received. It looks for a free buffer in the pool, copies the received frame into it, and then notifies the appropriate Protocol observer based on the EtherType. If no free buffer is available, the frame is silently discarded.
        void _handle(void* raw, size_t len) override {
            for(auto& buf : _buffer) {
                if(!buf.in_use) {
                    buf.in_use = true;
                    buf.size   = static_cast<unsigned int>(len);
                    std::memcpy(&buf.frame, raw, len);

                    // Notify observers based on the EtherType (protocol number)    
                    Protocol_Number prot = buf.frame.type;
                    this->notify(prot, &buf);
                    return;
                }
            }
            // pool esgotado — frame descartado silenciosamente
        }

        Address                        _address;
        Buffer<Ethernet::Frame>        _buffer[BUFFER_COUNT];
};