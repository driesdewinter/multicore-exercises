#ifndef __FIFO_H__
#define __FIFO_H__

#include <array>
#include <thread>
#include <mutex>

using namespace std::literals::chrono_literals;

template<typename T, std::size_t N = 65536>
class fifo {
public:
    void push(T&& value) {
        while (full())
            std::this_thread::sleep_for(1us);
        data[tail] = std::move(value);
        std::lock_guard<std::mutex> lock(mtx);
        tail = (tail + 1) % N;
    }

    T pop() {
        T ret;
        while (empty())
            std::this_thread::sleep_for(1us);
        ret = std::move(data[head]);
        std::lock_guard<std::mutex> lock(mtx);
        head = (head + 1) % N;
        return ret;
    }

private:
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return head == tail;
    }

    bool full() const {
        std::lock_guard<std::mutex> lock(mtx);
        return (tail + 1) % N == head;
    }

    mutable std::mutex mtx;
    std::array<T, N> data;
    std::size_t head = 0;
    std::size_t tail = 0;
};

#endif /* __FIFO_H__ */
