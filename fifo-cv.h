#ifndef __FIFO_H__
#define __FIFO_H__

#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T, std::size_t N = 65536>
class fifo {
public:
    void push(T&& value) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv_full.wait(lock, [&]() {
                return not full();
            });
        }

        data[tail] = std::move(value);

        {
            std::lock_guard<std::mutex> lock(mtx);
            tail = (tail + 1) % N;
        }
        cv_empty.notify_one();
    }

    T pop() {
        T ret;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv_empty.wait(lock, [&]() {
                return not empty();
            });
        }

        ret = std::move(data[head]);

        {
            std::lock_guard<std::mutex> lock(mtx);
            head = (head + 1) % N;
        }
        cv_full.notify_one();

        return ret;
    }

private:
    bool empty() const {
        return head == tail;
    }

    bool full() const {
        return (tail + 1) % N == head;
    }

    mutable std::mutex mtx;
    std::array<T, N> data;
    std::size_t head = 0;
    std::size_t tail = 0;
    std::condition_variable cv_full;
    std::condition_variable cv_empty;
};

#endif /* __FIFO_H__ */
