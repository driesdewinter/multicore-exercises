#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <cassert>

using namespace std::literals::chrono_literals;

template<class T>
class tsfifo {
    std::queue<T> m_data;
    mutable std::mutex m_mtx;
public:
    void push(T&& value) {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_data.push(std::move(value));
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_data.empty();
    }

    T pop() {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_data.empty()) return T{};
        T ret = std::move(m_data.back());
        m_data.pop();
        return ret;
    }
};

int main()
{
    std::vector<std::thread> threads;
    tsfifo<int> q;

    for (int i = 1; i <= 100; i++) {
        threads.emplace_back([&] {
            for (int j = 0; j < 10000; j++) {
                while (q.empty()) std::this_thread::sleep_for(1ms);
                assert(q.pop() > 0);
            }
        });
        threads.emplace_back([&] {
            for (int j = 0; j < 10000; j++) {
                q.push(i + j * 10);
            }
        });
    }

    for (auto& thread : threads)
        thread.join();

    return 0;
}
