#include <mutex>
#include <iostream>

template<typename T> class List{
    private:
        struct Node {
            T val;
            Node * next;
            Node(const T & d): val(d), next(nullptr) {}
        };

        Node * _head = nullptr;
        Node * _tail = nullptr;
        std::mutex _mutex;
    public:
        List() {}
        ~List() {
            std::lock_guard<std::mutex> lock(_mutex);
            Node * current = _head;
            while(current) {
                Node * next = current->next;
                delete current;
                current = next;
            }
        }

        void insert(T val) {
            std::lock_guard<std::mutex> lock(_mutex);
            Node * new_node = new Node(val);
            if(!_head) {
                _head = new_node;
                _tail = new_node;
            } else {
                _tail->next = new_node;
                _tail = new_node;
            }
        }

        T remove() {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!_head) throw std::runtime_error("List is empty");
            Node * old_head = _head;
            T val = old_head->val;
            _head = old_head->next;
            if(!_head) _tail = nullptr; // List is now empty
            delete old_head;
            return val;
        }

        bool empty() {
            std::lock_guard<std::mutex> lock(_mutex);
            return _head == nullptr;
        }
};