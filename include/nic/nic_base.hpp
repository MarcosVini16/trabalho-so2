#pragma once
#include "../ethernet.hpp"
#include "../utils/buffer.hpp"
#include "../observe/conditional.hpp"
#include "../utils/traits.hpp"

/*
 * Base class for Network Interface Cards (NICs) with conditional observation capabilities.
 */
class NICBase : public ConditionalObserved<Buffer<Ethernet::Frame>, Ethernet::Protocol> {
public:
    using Address         = Ethernet::Address;
    using Protocol_Number = Ethernet::Protocol;
    using Buffer          = ::Buffer<Ethernet::Frame>;
    using Observer        = ConditionalObserver<Buffer, Protocol_Number>;
    using Observed        = ConditionalObserved<Buffer, Protocol_Number>;

    static const unsigned int MTU = Ethernet::MTU; // Maximum Transmission Unit for Ethernet frames
    static const unsigned int SEND_BUFFERS = Traits<NICT>::SEND_BUFFERS; // Number of send buffers in the NIC
    static const unsigned int RECEIVE_BUFFERS = Traits<NICT>::RECEIVE_BUFFERS; // Number of receive buffers in the NIC
    static const unsigned int BUFFER_COUNT = SEND_BUFFERS + RECEIVE_BUFFERS; // Total number of buffers in the NIC

    virtual int     send(Buffer* buf) = 0;
    virtual Buffer* alloc(Address dst, Protocol_Number prot,
                          unsigned int size) = 0;
    virtual void    free(Buffer* buf) = 0;
    virtual int     receive(Buffer* buf, Address* src,
                            void* data, unsigned int size) = 0;
    virtual void    attach(Observer* obs, Protocol_Number prot) = 0;
    virtual void    detach(Observer* obs, Protocol_Number prot) = 0;
    virtual Address address() const = 0;

    virtual ~NICBase() = default;
};