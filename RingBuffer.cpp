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
    const size_t buffer_mask_; // size - 1 for bit computation
    Element* buffer_;

    //alignas: allocate these vars in independant cache lines
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> tail_; // producer location
    alignas(CACHE_LINE_SIZE) std::atomic<size_t> head_; // consumer location

public:
    //size must be 2**n
    RingBuffer(size_t size) 
        : buffer_mask_(size - 1), buffer_(new Element[size]), tail_(0), head_(0)
    {
        // slot reset
        for (size_t i = 0; i < size; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    ~RingBuffer() { delete[] buffer_;}
}