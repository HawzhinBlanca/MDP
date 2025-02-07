// LockFreeFifo.h
#pragma once

#include <atomic>
#include <vector>

template <typename T>
class LockFreeFifo
{
public:
    LockFreeFifo(size_t capacity)
        : capacity(capacity), buffer(capacity)
    {
        writeIndex.store(0);
        readIndex.store(0);
    }

    bool push(const T& item)
    {
        auto w = writeIndex.load(std::memory_order_relaxed);
        auto r = readIndex.load(std::memory_order_acquire);
        auto next = (w + 1) % capacity;
        if (next == r)
            return false; // full

        buffer[w] = item;
        writeIndex.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& outItem)
    {
        auto r = readIndex.load(std::memory_order_relaxed);
        auto w = writeIndex.load(std::memory_order_acquire);
        if (r == w)
            return false; // empty

        outItem = buffer[r];
        r = (r + 1) % capacity;
        readIndex.store(r, std::memory_order_release);
        return true;
    }

private:
    size_t capacity;
    std::vector<T> buffer;
    std::atomic<size_t> writeIndex, readIndex;
};