#include "../../include/components/component.hpp"

Component::Component(Protocol::Address address) : _protocol(&_nic), _communicator(&_protocol, address) {
    // Construtor com endereço, pode ser usado para inicializações comuns
    NICBase* nic_ptr = &_nic; // Upcast is failing
    _protocol.attach_nic(&_nic);
}

bool Component::send(const Message& msg, Protocol::Address dst) {
    return _communicator.send(&msg);
}

bool Component::receive(Message& msg, Protocol::Address& src) {
    return _communicator.receive(&msg);
}

Protocol::Address Component::address() const {
    return _communicator.address();
}