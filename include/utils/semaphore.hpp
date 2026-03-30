#include <semaphore>

#ifndef SEMAPHORE_HPP
#define SEMAPHORE_HPP

// A simple wrapper around C++20's counting_semaphore to provide a more traditional semaphore interface.
// (Needs to be implemented in a .cpp file)
class Semaphore {
    public:
        Semaphore(unsigned int count = 0) : sem(count) {}

        void p() {
            sem.acquire();
        }

        void v() {
            sem.release();
        }

    private:
        std::counting_semaphore<> sem; // Using C++20 counting_semaphore
};

#endif // SEMAPHORE_HPP