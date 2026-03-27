// Communication End-Point (for client classes)
#include "observe.hpp"
#include "message.hpp"
#include "utils/buffer.hpp"
#include "ethernet.hpp"
template<typename Channel>
class Communicator
    : public Concurrent_Observer<Buffer<Ethernet::Frame>,
                                 typename Channel::Port>
{
    using Port     = typename Channel::Port;
    using Buffer   = ::Buffer<Ethernet::Frame>;
    using Observer = Concurrent_Observer<Buffer, Port>;

    public:
        using Address = typename Channel::Address;

    public:
        Communicator(Channel* channel, Address address)
            : Observer(address.port),   // rank = porta
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
        // chamado pelo Protocol quando chega um frame na nossa porta
        void update(Port, Buffer* buf) override {
            Observer::update(_address.port, buf);
        }

        // necessário para o Ordered_List filtrar
        Port condition() const override {
            return _address.port;
        }

        Channel* _channel;
        Address  _address;
};