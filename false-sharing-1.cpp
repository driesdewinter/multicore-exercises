#include <iostream>
#include <thread>

int main() {
    struct {
        uint16_t x1;
        uint16_t x2;
    } x;

    std::thread t1([&]() {
        x.x1 = 0xAAAA;
    });
    std::thread t2([&]() {
        x.x2 = 0xBBBB;
    });
    t1.join();
    t2.join();

    std::cout << std::hex << x.x1 << x.x2 << "\n";

    return 0;
}

