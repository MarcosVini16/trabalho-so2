#include "../engine/shm_engine.hpp"
#include "../nic/nic.hpp"
#include "../protocol.hpp"
#include "../communicator.hpp"

/*
 * Base class for all component of an autonomous vehicle.
 * Capable of communicating with other components (processes) of the same 
 * vehicle through a shared memory NIC.
 */
class Component {
public:
    Component();
    virtual ~Component();

    virtual void init() = 0;
    virtual void run() = 0;

protected:
    NIC<ShmEngine> _nic;
    Protocol _protocol;
    Communicator _communicator;
};