// vehicle.hpp
#pragma once
#include <vector>
#include <memory>
#include <thread>
#include "components/component.hpp"
#include "components/gateway.hpp"
#include "utils/ports.hpp"

class Vehicle {
public:
    Vehicle(const std::string& iface, Ethernet::Address mac)
        : _mac(mac),
          _gateway({mac, Ports::GATEWAY}, iface)
    {}

    // registra um componente no veículo
    // cada componente roda como processo separado na entrega final
    // por ora, roda como thread para facilitar o teste
    template<typename C, typename... Args>
    C& add_component(Args&&... args) {
        auto ptr = std::make_unique<C>(std::forward<Args>(args)...);
        C& ref = *ptr;
        _components.push_back(std::move(ptr));
        return ref;
    }

    // inicia todos os componentes em threads separadas
    // substitua por fork()/exec() na entrega final
    void run() {
        for(auto& c : _components) {
            _threads.emplace_back([&c]() {
                Message msg;
                Protocol::Address from;
                while(true)
                    c->receive(msg, from);
            });
        }
        for(auto& t : _threads)
            t.join();
    }

    Gateway& gateway() { return _gateway; }

private:
    Ethernet::Address              _mac;
    Gateway                        _gateway;
    std::vector<std::unique_ptr<Component>> _components;
    std::vector<std::thread>       _threads;
};