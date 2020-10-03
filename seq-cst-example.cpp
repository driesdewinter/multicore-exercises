#include <thread>
#include <atomic>
#include <cassert>

int main()
{
    std::atomic<bool> x = false;
    std::atomic<bool> y = false;
    std::atomic<int> z = 0;
    std::thread t1([&] { x.store(true, std::memory_order_seq_cst); });
    std::thread t2([&] { y.store(true, std::memory_order_seq_cst); });
    std::thread t3([&] {
        while (not x.load(std::memory_order_seq_cst));
        if (y.load(std::memory_order_seq_cst))
            z++;
    });
    std::thread t4([&] {
        while (not y.load(std::memory_order_seq_cst));
        if (x.load(std::memory_order_seq_cst))
            z++;
    });
    t1.join(); t2.join(); t3.join(); t4.join();
    assert(z.load() != 0);
}
