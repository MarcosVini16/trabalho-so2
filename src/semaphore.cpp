#include "semaphore.hpp"
#include <semaphore>

Semaphore::Semaphore(unsigned int count) : sem(count) {}

void Semaphore::p() {
    sem.acquire();
}

void Semaphore::v() {
    sem.release();
}