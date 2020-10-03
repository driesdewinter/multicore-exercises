#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

int main() {
    int x = 0;
    std::mutex mtx;
    auto next = [&] {
        std::lock_guard<std::mutex> lock(mtx);
        return ++x;
    };

    std::mutex mtx_ready;
    std::condition_variable cv_ready;
    bool ready = false;

    std::thread t1([&]() {
        std::cout << next();

        {
            std::lock_guard<std::mutex> lock(mtx_ready);
            ready = true;
        }
        cv_ready.notify_all();
    });
    std::thread t2([&]() {
        {
            std::unique_lock<std::mutex> lock(mtx_ready);
            cv_ready.wait(lock, [&] {return ready;});
        }

        std::cout << next();
    });
    t1.join();
    t2.join();

    return 0;
}

