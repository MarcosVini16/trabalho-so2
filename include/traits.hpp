template<typename T> struct Traits {};

class NIC;
template<> struct Traits<NIC> {
    static const unsigned int SEND_BUFFERS = 4;
    static const unsigned int RECEIVE_BUFFERS = 4;
};

class Protocol;
template<> struct Traits<Protocol> {
    static const unsigned int ETHERNET_PROTOCOL_NUMBER = 0x0800; // IPv4 protocol number in Ethernet
};