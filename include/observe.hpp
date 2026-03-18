// Fundamentals for Observer X Observed
#include <semaphore>
// (For more info on Observer Pattern: https://refactoring.guru/design-patterns/observer)
template <typename T, typename Condition = void> class ConditionalDataObserver;
template <typename T, typename Condition = void> class ConditionallyDataObserved;

// Conditional Observer x Conditionally Observed with Data decoupled by a Semaphore
template<typename D, typename C = void> class ConcurrentObserver;

// Concurrent_Observed is the base class for all classes that want to be observed by Concurrent_Observer
template<typename D, typename C = void> class ConcurrentObserved {
    friend class ConcurrentObserver<D, C>; // to allow ConcurrentObserver to call update() and access _observers

    public:
        using ObservedData = D;
        using ObservingCondition = C;
        using Observers = Ordered_List<ConcurrentObserver<D, C>, C>;
    
    public:
        ConcurrentObserved() {}
        ~ConcurrentObserved() {}

        // Attach and detach observers to the observed object based on the observing condition (C)
        // (Is C being used here? It seems like we are not using C in the attach/detach functions. Should we consider using it to filter observers based on their observing condition?)
        void attach(ConcurrentObserver<D, C> * o, C c) {
            _observers.insert(o);
        }

        void detach(ConcurrentObserver<D, C> * o, C c) {
            _observers.remove(o);
        }

        bool notify(C c, D * d) {
            bool notified = false;
            for(Observers::Iterator obs = _observers.begin(); obs != _observers.end(); obs++) {
                if(obs->rank() == c) {
                    obs->update(c, d);
                    notified = true;
                }
            }
            return notified;
        }
    private:
        Observers _observers;
};
template<typename D, typename C> class ConcurrentObserver {
    friend class ConcurrentObserved<D, C>; // to allow ConcurrentObserved to call update() and access _data and _semaphore
    public:
        using ObservedData = D;
        using ObservingCondition = C;
    public:
        ConcurrentObserver(): _semaphore(0) {}
        ~ConcurrentObserver() {}

        // This function will be called by the observed object when it wants to notify the observer of a change. It will insert the data into the _data list and release the semaphore to unblock any thread that is waiting for data in the updated() function.
        void update(C c, D * d) {
            _data.insert(d);
            _semaphore.v();
        }

        // This function will block until a notification is triggered and data is available in the _data list. It will return the data that was notified by the observed object.
        D * updated() {
            _semaphore.p();
            return _data.remove();
        }
    private:
        // Could use a counting_semaphore from c++20 (maybe change it later)
        Semaphore _semaphore;
        // List is a simple linked list implementation (need to be defined elsewhere) - couldn't we use std::list instead? 
        // Note: The List class should be thread-safe since it will be accessed by multiple threads (ConcurrentObservers) when they call update() and updated().
        List<D> _data;
};