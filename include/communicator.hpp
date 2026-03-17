// Communication End-Point (for client classes)
class Message; // Forward declaration of Message class (need to be defined elsewhere)
template <typename Channel> // Forward declaration of Channel template class (need to be defined elsewhere)
class Communicator: public Concurrent_Observer<typename Channel::Observer::Observed_Data, typename Channel::Observer::Observing_Condition> {

    typedef Concurrent_Observer<typename Channel::Observer::Observed_Data, typename Channel::Observer::Observing_Condition> Observer;

    public:
        typedef typename Channel::Buffer Buffer;
        typedef typename Channel::Address Address;

    public:
        Communicator(Channel * channel, Address address): _channel(channel), _address(address) {
            _channel->attach(this, address);
        }

        ~Communicator() { Channel::detach(this, _address); }

        bool send(const Message * message) {
            return (_channel->send(_address, Channel::Address::BROADCAST, message->data(), message->size()) > 0);
        }

        bool receive(Message * message) {
            Buffer * buf = Observer::updated(); // block until a notification is triggered
            Channel::Address from;
            int size = _channel->receive(buf, &from, message->data(), message->size());
            // . . .
            if(size > 0)
            return true;
        }

    private:
        void update(typename Channel::Observed * obs, typename

        Channel::Observer::Observing_Condition c, Buffer * buf) {
            Observer::update(c, buf); // releases the thread waiting for data
        }

    private:
        Channel * _channel;
        Address _address;
};