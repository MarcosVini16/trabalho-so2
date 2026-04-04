// gateway.hpp
#pragma once
#include "../nic/nic.hpp"
#include "../engine/raw_socket_engine.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"
#include "../utils/ports.hpp"

class Gateway {
public:
    Gateway(const std::string& iface)
        : _gw_nic(iface),
          _protocol(&_gw_nic),
          _communicator(&_protocol,
                        Protocol::Address{_gw_nic.address(), Ports::GATEWAY})
    {
        //_protocol.attach_nic(&_gw_nic);
    }

    ~Gateway() = default;

    bool send(const Message& msg) {
        return _communicator.send(&msg);
    }

    bool receive(Message& msg) {
        return _communicator.receive(&msg);
    }

    Protocol::Address address() const {
        return _communicator.address();
    }

    Ethernet::Address nic_address() const {
        return _gw_nic.address();
    }

    void attach_nic(NICBase* nic) {
        _protocol.attach_nic(nic);
    }

private:
    NIC<RawSocketEngine> _gw_nic;
    Protocol             _protocol;
    Communicator         _communicator;
};