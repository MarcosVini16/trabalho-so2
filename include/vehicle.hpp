// vehicle.hpp (simplificado)
#include "protocol.hpp"
#include "engine/engine.hpp"
#include "components/gateway.hpp"
#include "components/component.hpp"
#include "components/time_client.hpp"
#include "utils/ports.hpp"
#include <vector>
class Vehicle {
public:
    Vehicle(const std::string& iface)
        : _gateway(iface) 
    {
        // a chave é derivada do MAC da interface — único por VM
        _key = _gateway.key();
    }

    template<typename C, typename... Args>
    C& add_component(Protocol::Port port, Args&&... args) {
        Protocol::Address addr{_gateway.address().paddr, port};
        // Component recebe a chave e o MAC — Engine faz o resto
        auto ptr = std::make_unique<C>(addr, _key,
                                       _gateway.address().paddr,
                                       std::forward<Args>(args)...);
        C& ref = *ptr;
        _components.push_back(std::move(ptr));
        return ref;
    }

    Ethernet::Address mac_address() const {
        return _gateway.address().paddr;
    }

    key_t shm_key() const {
        return _key;
    }

    Gateway& gateway() {
        return _gateway;
    }

private:
    static key_t _make_key(Ethernet::Address mac) {
        // usa os últimos 4 bytes do MAC como base da chave
        key_t k = 0;
        std::memcpy(&k, mac.bytes + 2, 4);
        return k ? k : 1;
    }

    Gateway _gateway;
    key_t   _key;
    key_t clock_key; // chave para o segmento de memória compartilhada do relógio
    std::vector<std::unique_ptr<Component>> _components;
};