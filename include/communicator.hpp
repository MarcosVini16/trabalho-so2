// Communication End-Point (for client classes)
#include "observe/concurrent.hpp"
#include "message.hpp"
#include "utils/buffer.hpp"
#include "ethernet.hpp"
#include "protocol.hpp"

class Communicator
    : public ConcurrentObserver<Buffer<Ethernet::Frame>,
                                 typename Protocol::Port>
{
    using Port     = typename Protocol::Port;
    using Buffer   = ::Buffer<Ethernet::Frame>;
    using Observer = ConcurrentObserver<Buffer, Port>;

    public:
        using Address = typename Protocol::Address;

    public:
        Communicator(Protocol* channel, Address address)
            :
            _channel(channel),
            _address(address)
        {
            _channel->attach(this, address.port);
        }

        ~Communicator() {
            _channel->detach(this, _address.port);
        }

        bool send(const Message* msg) {
            return _channel->send(
                _address,
                Address::BROADCAST(),
                msg->data(),
                msg->size()
            ) > 0;
        }

        bool receive(Message* msg) {
            // bloqueia até um buffer chegar via semáforo
            Buffer* buf = Observer::updated();

            Address from;
            int size = _channel->receive(buf, &from, msg->data(), msg->size());
            msg->set_size(size);

            _channel->free(buf);   // devolve o buffer ao pool da NIC
            return size > 0;
        }

    private:
        void update(Port, Buffer* buf) override {
            Observer::update(_address.port, buf);
        }

        // necessário para o Ordered_List filtrar
        Port condition() const {
            return _address.port;
        }

        Protocol* _channel;
        Address  _address;
};