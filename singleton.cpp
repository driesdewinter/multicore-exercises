#include <memory>
#include <thread>
#include <vector>
#include <iostream>
#include <mutex>
#include <atomic>

class singleton {
private:
    static std::unique_ptr<singleton> instance;
    static std::atomic<singleton*> pinstance;
    static std::mutex instance_mtx;
public:
    static singleton& get() {
        singleton* p = pinstance.load(std::memory_order_acquire);
        if (not p) {
            std::lock_guard<std::mutex> lock(instance_mtx);
            if (not instance) {
                instance = std::make_unique<singleton>();
                pinstance.store(instance.get(), std::memory_order_release);
            }
            else {
                std::cout << "someone else is doing it\n";
            }
        }
        return *p;
    }

    // ...
    singleton() { std::cout << "constructor\n"; }
    ~singleton() { std::cout << "destructor\n"; }
};

std::unique_ptr<singleton> singleton::instance;
std::atomic<singleton*> singleton::pinstance = nullptr;
std::mutex singleton::instance_mtx;

int main()
{
    std::vector<std::thread> threads;

    for (int i = 1; i < 100; i++) {
        threads.emplace_back([&] {
           singleton::get();
        });
    }
    for (auto& thread : threads)
        thread.join();
    return 0;
}
