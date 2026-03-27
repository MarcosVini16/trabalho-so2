#include "./nic.hpp"
#include "./protocol.hpp"
#include "./communicator.hpp"
#include "component.hpp"
#include "./engine/raw_socket_engine.hpp"

#ifndef GATEWAY_COMPONENT_HPP
#define GATEWAY_COMPONENT_HPP

class Gateway_Component : public Component {
    public:
        Gateway_Component(const std::string& iface)
            : Component(),
              nic(new RawSocketEngine(iface)),
              protocol(&nic),
              communicator(&protocol, nic.address())
        {}

    private:
        NIC<RawSocketEngine> nic;
        Protocol<NIC<RawSocketEngine>> protocol;
        Communicator<Protocol<NIC<RawSocketEngine>>> communicator;
};

#endif // GATEWAY_COMPONENT_HPP