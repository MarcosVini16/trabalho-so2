#pragma once
#include "component.hpp"
#include "../protocol.hpp"
#include "engine/raw_socket_engine.hpp"
#include <string>

/*
 * A gateway component for handling communication between different 
 * vehicles (VMs) in a multi-vehicle system. 
 * It uses a raw socket engine to send and receive Ethernet frames.
 */
class Gateway : public Component {
public:
    Gateway(Protocol::Address address, const std::string& iface) 
        : Component(address), 
        gw_nic(iface) 
    {
        _protocol.attach_nic(&gw_nic);
    }
    
    ~Gateway() override = default;
private:
    NIC<RawSocketEngine> gw_nic;
};