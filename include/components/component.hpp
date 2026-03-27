#include "./engine/engine.hpp"
#include "./nic.hpp"
#include "./protocol.hpp"
#include "./communicator.hpp"

#ifndef COMPONENT_HPP
#define COMPONENT_HPP

class Component {

    using NIC = NIC<Engine>;
    using Protocol = Protocol<NIC>;
    using Communicator = Communicator<Protocol>;

    public:
        virtual ~Component() = default;

    private:
        NIC nic;
        Protocol protocol;
        Communicator communicator;
};

#endif // COMPONENT_HPP