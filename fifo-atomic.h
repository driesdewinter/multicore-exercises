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
        data[tail] = std::move(value);
        tail = (tail + 1) % N;
    }

    T pop() {
        T ret;
        while (empty())
            std::this_thread::sleep_for(1us);
        ret = std::move(data[head]);
        head = (head + 1) % N;
        return ret;
    }

private:
    bool empty() const {
        return head == tail;
    }

    bool full() const {
        return (tail + 1) % N == head;
    }

    std::array<T, N> data;
    std::atomic<std::size_t> head = 0;
    std::atomic<std::size_t> tail = 0;
};

#endif /* __FIFO_H__ */
