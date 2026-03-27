#include <list>
#include <mutex>

/*
 * A simple thread-safe ordered list implementation.
 */
template<typename O, typename C>
class Ordered_List {
    public:
        using Observer = O;
        using Condition = C;

    public:
        Ordered_List() {}
        ~Ordered_List() {}

        /**
         * Inserts an observer into the list in the correct position based on its rank.
         */
        void insert(Observer o) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _list.begin();
            while(it != _list.end() && it->rank() < o.rank()) {
                ++it;
            }
            _list.insert(it, o);
        }

        /**
         * Removes an observer from the list.
         */
        void remove(Observer o) {
            std::lock_guard<std::mutex> lock(_mutex);
            _list.remove(o);
        }

        /**
         * Returns an iterator to the beginning of the list.
         */
        typename std::list<Observer>::iterator begin() {
            return _list.begin();
        }

        /**
         * Returns an iterator to the end of the list.
         */
        typename std::list<Observer>::iterator end() {
            return _list.end();
        }

    private:
        std::list<Observer> _list;
        std::mutex _mutex;
};