// Observable pattern with conditional observers.
// Observers can specify a condition (C) that determines when they should be notified of changes in the observed data (D). 
// The ConditionalObserved class manages a list of observers and notifies them
// (Normally, the condition will be the Protocol_Number in the NIC, so that Protocols can register to be notified when a frame with their EtherType is received.)
#pragma once
#include "../utils/ordered_list.hpp"

/*
 * A conditional observer for observing changes in observed data based on a condition.
 */
template<typename D, typename C>
class ConditionalObserver {
public:
    virtual void update(C c, D* d) = 0;
    virtual C condition() const = 0;
    virtual ~ConditionalObserver() = default;
};

/*
 * A conditional observed for observing changes in observed data based on a condition.
 */
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
            if(it->condition() == c) {
                it->update(c, d);
                notified = true;
            }
        }
        return notified;
    }

private:
    Observers _observers;
};