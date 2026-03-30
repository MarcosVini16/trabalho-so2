#pragma once
template<typename T> struct Traits {};

// Specializations for specific types
class NICT;
template<> struct Traits<NICT> {
    static const unsigned int SEND_BUFFERS = 4;
    static const unsigned int RECEIVE_BUFFERS = 4;
};

// Specialization for Protocol to define its Ethernet protocol number
class ProtocolT;
template<> struct Traits<ProtocolT> {
    static const unsigned int ETHERNET_PROTOCOL_NUMBER = 0x8888; // example EtherType for our custom protocol
};