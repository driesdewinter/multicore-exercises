#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

using namespace std::literals::chrono_literals;

struct data_t
{
    volatile int a = 0;
};
std::array<data_t, 4> data;

int main() {
    volatile bool done = false;

    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < data.size(); i++) {
        threads.emplace_back(std::thread([&done, i]() {
            while (not done)
                data[i].a++;
        }));
    }

    std::this_thread::sleep_for(1s);
    done = true;

    for (auto& t : threads)
        t.join();

    std::cout << "data@" << data.data() << "\n";

    for (std::size_t i = 0; i < data.size(); i++)
        std::cout << "data[" << i << "].a=" << data[i].a << "\n";

    return 0;
}
