#include "component.hpp"
#include "engine/raw_socket_engine.hpp"

/*
 * A gateway component for handling communication between different 
 * vehicles (VMs) in a multi-vehicle system. 
 * It uses a raw socket engine to send and receive Ethernet frames.
 */
class Gateway : public Component {
public:
    Gateway() : Component() {}
    ~Gateway() override = default;
private:
    NIC<RawSocketEngine> gw_nic;
};