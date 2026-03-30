#include "../../include/engine/shm_engine.hpp"

ShmEngine::ShmEngine() {
    // This class will be implemented in the future to allow communication between processes using shared memory.
}

ShmEngine::~ShmEngine() {
    // Clean up shared memory resources here
}

int ShmEngine::_send(const void* buf, size_t len) {
    // Implement the logic to write data to the shared memory region here
    // For now, we will just return -1 to indicate that this is not yet implemented
    return -1;
}

void ShmEngine::_handle(void* buf, size_t len) {
    // Implement the logic to read data from the shared memory region and notify observers here
    // For now, this function does nothing
}