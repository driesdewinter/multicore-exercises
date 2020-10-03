#include <iostream>
#include <thread>
#include <mutex>

int main() {
    int x = 0;
    std::mutex mtx;
    std::thread t1([&]() {
        int a = 0;
        {
            std::lock_guard<std::mutex> lock(mtx);
            a = ++x;
        }
        std::cout << a;
    });
    std::thread t2([&]() {
        int b = 0;
        {
            std::lock_guard<std::mutex> lock(mtx);
            b = ++x;
        }
        std::cout << b;
    });
    t1.join();
    t2.join();

    return 0;
}

