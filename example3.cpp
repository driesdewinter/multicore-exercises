#include <iostream>
#include <thread>
#include <mutex>

int main() {
    int x = 0;
    std::mutex mtx;
    auto next = [&] {
        std::lock_guard<std::mutex> lock(mtx);
        return ++x;
    };
    std::thread t1([&]() {
        std::cout << next();
    });
    std::thread t2([&]() {
        std::cout << next();
    });
    t1.join();
    t2.join();

    return 0;
}

