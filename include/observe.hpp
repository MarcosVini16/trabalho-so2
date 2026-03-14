// Fundamentals for Observer X Observed
template <typename T, typename Condition = void> class Conditional_Data_Observer;
template <typename T, typename Condition = void> class Conditionally_Data_Observed;

// Conditional Observer x Conditionally Observed with Data decoupled by a Semaphore
template<typename D, typename C = void> class Concurrent_Observer;

// Concurrent_Observed is the base class for all classes that want to be observed by Concurrent_Observer
template<typename D, typename C = void> class Concurrent_Observed {
    friend class Concurrent_Observer<D, C>; // to allow Concurrent_Observer to call update() and access _observers

    public:
        typedef D Observed_Data;
        typedef C Observing_Condition;
        typedef Ordered_List<Concurrent_Observer<D, C>, C> Observers;
    
    public:
        Concurrent_Observed() {}
        ~Concurrent_Observed() {}

        void attach(Concurrent_Observer<D, C> * o, C c) {
            _observers.insert(o);
        }

        void detach(Concurrent_Observer<D, C> * o, C c) {
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

template<typename D, typename C> class Concurrent_Observer {
    friend class Concurrent_Observed<D, C>; // to allow Concurrent_Observed to call update() and access _data and _semaphore
    public:
        typedef D Observed_Data;
        typedef C Observing_Condition;
    public:
        Concurrent_Observer(): _semaphore(0) {}
        ~Concurrent_Observer() {}

        void update(C c, D * d) {
            _data.insert(d);
            _semaphore.v();
        }

        D * updated() {
            _semaphore.p();
            return _data.remove();
        }
    private:
        Semaphore _semaphore;
        List<D> _data;
};