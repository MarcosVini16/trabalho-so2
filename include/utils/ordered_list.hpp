#include <list>
#include <mutex>

/*
 * A simple thread-safe ordered list implementation.
 */
template<typename O, typename C>
class Ordered_List {
public:
    // O observer é um ponteiro, C é a condição/rank
    void insert(O* obj, C rank) {
        std::lock_guard<std::mutex> lock(_mutex);
        // guarda o par (rank, ponteiro)
        auto it = _list.begin();
        while(it != _list.end() && it->first < rank)
            ++it;
        _list.insert(it, {rank, obj});
    }

    void remove(O* obj) {
        std::lock_guard<std::mutex> lock(_mutex);
        _list.remove_if([obj](const auto& pair) {
            return pair.second == obj;
        });
    }

    // iterador expõe rank() e operator->
    class Iterator {
    public:
        using Inner = typename std::list<std::pair<C, O*>>::iterator;

        Iterator(Inner it) : _it(it) {}

        C    rank()  const { return _it->first; }
        O*   operator->()  { return _it->second; }
        O&   operator*()   { return *_it->second; }

        Iterator& operator++() { ++_it; return *this; }
        bool operator!=(const Iterator& o) const { return _it != o._it; }

    private:
        Inner _it;
    };

    Iterator begin() { return Iterator(_list.begin()); }
    Iterator end()   { return Iterator(_list.end()); }

private:
    std::list<std::pair<C, O*>> _list;
    std::mutex                  _mutex;
};