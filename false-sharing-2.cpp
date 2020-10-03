#include <iostream>
#include <thread>

int main() {
    union
    {
        struct {
            uint32_t x1:20;
            uint32_t x2:12;
        };
        uint32_t all = 0;
    } x;

    std::thread t1([&]() {
        x.x1 = 0xAAAAA;
    });
    std::thread t2([&]() {
        x.x2 = 0xBBB;
    });
    t1.join();
    t2.join();

    std::cout << std::hex << x.all << "\n";

    return 0;
}

