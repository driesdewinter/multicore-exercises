#include "test_main.h"
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

struct test
{
    virtual ~test() {}

    virtual void op1() = 0;
    virtual void op2() = 0;
    virtual bool eval() = 0;
};

template<class T>
bool run(T t)
{
    auto t0 = std::chrono::steady_clock::now();
    std::thread thr1([&]() { for (int i = 0; i < s_iterations; i++) t.op1(); });
    std::thread thr2([&]() { for (int i = 0; i < s_iterations; i++) t.op2(); });
    thr1.join();
    thr2.join();
    auto t1 = std::chrono::steady_clock::now();
    std::cout << "It took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms.\n";
    return t.eval();
}

#define REGISTER_TEST(impl) named_test s_##impl(#impl, []() { return run(impl()); });


struct contended_int : test
{
    void op1() override
    {
        X++;
    }
    void op2() override
    {
        X--;
    }
    bool eval() override
    {
        return X == 0;
    }
    volatile int X = 0;
};
REGISTER_TEST(contended_int);

struct uncontended_int : test
{
    void op1() override
    {
        X++;
    }
    void op2() override
    {
        Y++;
    }
    bool eval() override
    {
        return X == s_iterations and Y == s_iterations;
    }
    volatile int X = 0;
    char padding[256];
    volatile int Y = 0;
};
REGISTER_TEST(uncontended_int);


struct atomic_fetch_add_relaxed : test
{
    void op1() override
    {
        X.fetch_add(1, std::memory_order_relaxed);
    }
    void op2() override
    {
        X.fetch_add(-1, std::memory_order_relaxed);
    }
    bool eval() override
    {
        return X.load() == 0;
    }
    std::atomic<int> X = 0;
};
REGISTER_TEST(atomic_fetch_add_relaxed);

struct atomic_fetch_add_seq_cst : test
{
    void op1() override
    {
        X++;
    }
    void op2() override
    {
        X--;
    }
    bool eval() override
    {
        return X.load() == 0;
    }
    std::atomic<int> X = 0;
};
REGISTER_TEST(atomic_fetch_add_seq_cst);

struct atomic_load_store_relaxed : test
{
    void op1() override
    {
        X.store(X.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    }
    void op2() override
    {
        X.store(X.load(std::memory_order_relaxed) - 1, std::memory_order_relaxed);
    }
    bool eval() override
    {
        return X.load() == 0;
    }
    std::atomic<int> X = 0;
};
REGISTER_TEST(atomic_load_store_relaxed);

struct atomic_load_store_seq_cst : test
{
    void op1() override
    {
        X = X + 1;
    }
    void op2() override
    {
        X = X - 1;
    }
    bool eval() override
    {
        return X.load() == 0;
    }
    std::atomic<int> X = 0;
};
REGISTER_TEST(atomic_load_store_seq_cst);
