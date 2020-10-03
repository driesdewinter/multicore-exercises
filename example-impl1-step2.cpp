#include <iostream>
#include <sstream>
#include <vector>
#include <thread>

int main() {
    std::size_t number_of_threads = 10;

    std::vector<std::thread> threads(number_of_threads);
    std::vector<std::ostringstream> oss(number_of_threads);

    // In parallelized code stream to os i.s.o. std::cout
    for (std::size_t i = 0; i < number_of_threads; i++) {
        threads[i] = std::thread([&, &os = oss[i]]() {
            os << 1 << 2 << "\n";
        });
    }

    // After joining, copy all output to std::cout sequentially
    for (std::size_t i = 0; i < number_of_threads; i++) {
        threads[i].join();
        std::cout << oss[i].str();
    }
    return 0;
}
