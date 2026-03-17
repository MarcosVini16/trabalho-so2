template<typename O, typename C = void> class Ordered_List {
    private:
        struct Node {
            O val;
            Node * next;
            Node(const O & d): val(d), next(nullptr) {}
        };

        Node * _head = nullptr;
        std::mutex _mutex;
    public:
        Ordered_List() {}
        ~Ordered_List() {
            std::lock_guard<std::mutex> lock(_mutex);
            Node * current = _head;
            while(current) {
                Node * next = current->next;
                delete current;
                current = next;
            }
        }

        void insert(O val) {
            std::lock_guard<std::mutex> lock(_mutex);
            Node * new_node = new Node(val);
            if(!_head || new_node->val.rank() < _head->val.rank()) {
                new_node->next = _head;
                _head = new_node;
            } else {
                Node * current = _head;
                while(current->next && current->next->val.rank() < new_node->val.rank()) {
                    current = current->next;
                }
                new_node->next = current->next;
                current->next = new_node;
            }
        }

        void remove(O val) {
            std::lock_guard<std::mutex> lock(_mutex);
            if(!_head) return; // List is empty
            if(_head->val == val) {
                Node * old_head = _head;
                _head = old_head->next;
                delete old_head;
                return;
            }
            Node * current = _head;
            while(current->next && current->next->val != val) {
                current = current->next;
            }
            if(current->next) { // Found the node to remove
                Node * node_to_remove = current->next;
                current->next = node_to_remove->next; // Bypass the node to remove
                delete node_to_remove; // Free the memory of the removed node
            }
        }

        class Iterator {
            private:
                Node * _current;

            public:
                Iterator(Node * start): _current(start) {}

                O & operator*() { return _current->val; }
                Iterator & operator++() { 
                    if(_current) _current = _current->next; 
                    return *this; 
                }
                bool operator!=(const Iterator & other) const { return _current != other._current; }
        };
        
        Iterator begin() { return Iterator(_head); }
        Iterator end() { return Iterator(nullptr); }
};