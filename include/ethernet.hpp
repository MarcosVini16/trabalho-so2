// Network
class Ethernet; // all necessary definitions and formats
template <typename Engine>

class NIC: public Ethernet, public Conditional_Data_Observed<Buffer<Ethernet::Frame>, Ethernet::Protocol>, private Engine {
    public:
        static const unsigned int BUFFER_SIZE =
            Traits<NIC>::SEND_BUFFERS * sizeof(Buffer<Ethernet::Frame>) +
            Traits<NIC>::RECEIVE_BUFFERS * sizeof(Buffer<Ethernet::Frame>);
        typedef Ethernet::Address Address;
        typedef Ethernet::Protocol Protocol_Number;
        typedef Conditional_Data_Observer<Buffer<Ethernet::Frame>, Ethernet::Protocol> Observer;
        typedef Conditionally_Data_Observed<Buffer<Ethernet::Frame>, Ethernet::Protocol> Observed;
    protected:
        NIC();
    public:
        ~NIC();
    int send(Address dst, Protocol_Number prot, const void * data, unsigned int size);
    int receive(Address * src, Protocol_Number * prot, void * data, unsigned int size);
    Buffer<Ethernet::Frame> * alloc(Address dst, Protocol_Number prot, unsigned int size);
    int send(Buffer<Ethernet::Frame> * buf);
    void free(Buffer<Ethernet::Frame> * buf);
    int unmarshal(Buffer<Ethernet::Frame> * buf, Address * src, Address * dst, void * data,
    unsigned int size);
    const Address & address();
    void address(Address address);
    const Statistics & statistics();
    void attach(Observer * obs, Protocol_Number prot); // possibly inherited
    void detach(Observer * obs, Protocol_Number prot); // possibly inherited
  private:
    Statistics _statistics;
    Buffer<Ethernet::Frame> _buffer[BUFFER_SIZE];
};