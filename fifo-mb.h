#ifndef __FIFO_H__
#define __FIFO_H__

#include <array>
#include <thread>
#include <atomic>

using namespace std::literals::chrono_literals;

template<typename T, std::size_t N = 65536>
class fifo {
public:
    void push(T&& value) {
        while (full())
            std::this_thread::sleep_for(1us);
        auto t = tail.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        data[t] = std::move(value);
        std::atomic_thread_fence(std::memory_order_release);
        tail.store((t + 1) % N, std::memory_order_relaxed);
    }

    T pop() {
        T ret;
        while (empty())
            std::this_thread::sleep_for(1us);
        auto h = head.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        ret = std::move(data[head]);
        std::atomic_thread_fence(std::memory_order_release);
        head.store((h + 1) % N, std::memory_order_relaxed);
        return ret;
    }

private:
    bool empty() const {
        return head.load(std::memory_order_relaxed) == tail.load(std::memory_order_relaxed);
    }

    bool full() const {
        return (tail.load(std::memory_order_relaxed) + 1) % N == head.load(std::memory_order_relaxed);
    }

    std::array<T, N> data;
    std::atomic<std::size_t> head = 0;
    std::atomic<std::size_t> tail = 0;
};

#endif /* __FIFO_H__ */
