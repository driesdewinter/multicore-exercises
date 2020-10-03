#include <thread>
#include <atomic>
#include <cassert>

int main()
{
    int data = 0;
    std::atomic<bool> ready = false;
    std::thread t1([&] {
        data = 42;
        ready.store(true, std::memory_order_release);
    });
    std::thread t2([&] {
        while (not ready.load(std::memory_order_acquire));
        assert(data == 42);
    });
    t1.join(); t2.join();
}
