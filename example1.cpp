#include <iostream>
#include <thread>

int main() {
    int x = 0;
    std::thread t1([&]() {
        int a = ++x;
        std::cout << a;
    });
    std::thread t2([&]() {
        int b = ++x;
        std::cout << b;
    });
    t1.join();
    t2.join();

    return 0;
}

