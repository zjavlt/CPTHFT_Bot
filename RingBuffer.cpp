#include <atomic>
#include <vector>
#include <cstddef>
#include <thread>
#include <iostream>


//cache size (prevents core conflicts from false sharing)
constexpr size_t CACHE_LINE_SIZE = 64;

template<typename T>
class RingBuffer {
private:
    //each buffer slot
    struct Element {
        T data;

        //slot sequence
        // initially index
        // increase by buffer size
        std::atomic<size_t> sequence;

        //padding: defends slot cache loss from neighbor computations
        char padding[CACHE_LINE_SIZE - sizeof(T) - sizeof(std::atomic<size_t>)];
    };

    //member var
    const size_t
}