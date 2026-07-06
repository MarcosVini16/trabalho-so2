// observe/concurrent_observer.hpp
#pragma once
#include "concurrent.hpp"    // de onde vêm Semaphore e List
#include "conditional.hpp"
#include <chrono>

// Desacopla recepção de processamento: a thread que notifica só
// enfileira + sinaliza; a thread dona consome bloqueando.
template<typename D, typename C>
class ConcurrentObserver : public ConditionalObserver<D, C> {
public:
    explicit ConcurrentObserver(C cond) : _condition(cond), _semaphore(0) {}

    ~ConcurrentObserver() override {
        while (!_data.empty()) delete _data.remove();   // drena o que sobrou
    }

    // chamado pela thread de recepção (Communicator::notify)
    void update(C /*c*/, D* d) override {
        _data.insert(d);
        _semaphore.v();
    }

    C condition() const override { return _condition; }

    D* updated() {                                  // bloqueia até chegar algo
        _semaphore.p();
        return _data.empty() ? nullptr : _data.remove();
    }

    D* updated_for(std::chrono::milliseconds t) {   // com timeout
        if (!_semaphore.try_p_for(t)) return nullptr;
        return _data.empty() ? nullptr : _data.remove();
    }

private:
    C          _condition;
    Semaphore  _semaphore;
    List<D*>   _data;
};