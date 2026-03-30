#pragma once
#include "../utils/ordered_list.hpp"

template<typename D, typename C>
class ConditionalObserver {
public:
    virtual void update(C c, D* d) = 0;
    virtual C condition() const = 0;
    virtual ~ConditionalObserver() = default;
};

template<typename D, typename C>
class ConditionalObserved {
    using Observer = ConditionalObserver<D, C>;
    using Observers = Ordered_List<Observer, C>;

public:
    void attach(Observer* obs, C c) {
        _observers.insert(obs, c);
    }

    void detach(Observer* obs, C c) {
        _observers.remove(obs);
    }

    bool notify(C c, D* d) {
        bool notified = false;
        for(auto it = _observers.begin(); it != _observers.end(); ++it) {
            if(it.rank() == c) {
                it->update(c, d);
                notified = true;
            }
        }
        return notified;
    }

private:
    Observers _observers;
};
