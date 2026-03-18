#include <mutex>
#include <list>

template<typename T> class List{
    public:
        List() {}
        ~List() {}

        void insert(T* obj) {
            std::lock_guard<std::mutex> lock(_mutex);
            _list.push_back(obj);
        }

        T * remove() {
            std::lock_guard<std::mutex> lock(_mutex);
            if(_list.empty()) {
                throw std::runtime_error("List is empty");
            }
            T * obj = _list.front();
            _list.pop_front();
            return obj;
        }

        bool empty() {
            std::lock_guard<std::mutex> lock(_mutex);
            return _list.empty();
        }

        size_t size() {
            std::lock_guard<std::mutex> lock(_mutex);
            return _list.size();
        }

    private:
        std::list<T *> _list;
        std::mutex _mutex;
};