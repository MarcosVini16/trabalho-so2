#include <list>
#include <mutex>

template<typename O, typename C = void> class Ordered_List {
    public:
        using Observer = O;
        using Condition = C;
        class Iterator {
            public:
                Iterator(typename std::list<Observer>::iterator it): _it(it) {}
                Observer & operator*() { return *_it; }
                Observer * operator->() { return &(*_it); }
                Iterator & operator++() { ++_it; return *this; }
                bool operator!=(const Iterator & other) const { return _it != other._it; }
            private:
                typename std::list<Observer>::iterator _it;
        };
    public:
        Ordered_List() {}
        ~Ordered_List() {}
        void insert(Observer * o) {
            std::lock_guard<std::mutex> lock(_mutex);
            _list.push_back(*o);
        }
        void remove(Observer * o) {
            std::lock_guard<std::mutex> lock(_mutex);
            _list.remove(*o);
        }

        Iterator begin() { return Iterator(_list.begin()); }
        Iterator end() { return Iterator(_list.end()); }
};