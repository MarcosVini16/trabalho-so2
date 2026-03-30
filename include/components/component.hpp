#pragma once
#include "../nic/nic_base.hpp"
#include "../nic/nic.hpp"
#include "../engine/shm_engine.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"

/*
 * Base class for all component of an autonomous vehicle.
 * Capable of communicating with other components (processes) of the same 
 * vehicle through a shared memory NIC.
 */
class Component {
public:

    Component(Protocol::Address address);

    virtual ~Component();

    bool send(const Message& msg);

    bool receive(Message& msg);

    Protocol::Address address() const;

protected:
    NIC<ShmEngine> _nic;
    Protocol _protocol;
    Communicator _communicator;
};