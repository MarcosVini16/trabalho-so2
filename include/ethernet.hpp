// ethernet.hpp
#pragma once
#include <cstdint>
#include <cstring>

class Ethernet {
    public:
        // The max payload size of an Ethernet frame.
        static const unsigned int MTU = 1500;

        struct Address {
            // The 6-byte MAC address.
            uint8_t bytes[6];
 
            bool operator==(const Address& o) const {
                return std::memcmp(bytes, o.bytes, 6) == 0;
            }
            bool operator!=(const Address& o) const { return !(*this == o); }

            // A special address that is used to broadcast to all hosts on the local network.
            static Address BROADCAST() {
                return Address{{0xff,0xff,0xff,0xff,0xff,0xff}};
            }
        };
        // Identifies the protocol of the payload. For example, 0x0800 for IPv4, 0x0806 for ARP, etc.
        using Protocol = uint16_t;   // EtherType

        // The structure of an Ethernet frame. The __attribute__((packed)) ensures that there is no padding between the fields.
        struct Frame {
            Address  dst;
            Address  src;
            Protocol type;
            uint8_t  data[MTU];
        } __attribute__((packed));
};