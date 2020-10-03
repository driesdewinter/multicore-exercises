#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <map>
#include <cstring>
#include <atomic>

class repetitive_task
{
public:
  template<class F>
  repetitive_task(F&& f) : m_f(f) {
    m_twister.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    m_thread = std::thread([this]() {
      for (;;)
      {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_cv.wait(lk, [this]{return m_ready or m_stop;});
        if (m_stop) return;

        while (m_twister() % 8 != 0) {}  // Short, random delay

        m_f();

        m_ready = false;
        m_done = true;
        lk.unlock();
        m_cv.notify_all();
      }
    });
  }

  ~repetitive_task()
  {
    {
      std::lock_guard<std::mutex> lk(m_mtx);
      m_stop = true;
    }
    m_cv.notify_all();
    m_thread.join();
  }

  void go()
  {
    {
      std::lock_guard<std::mutex> lk(m_mtx);
      m_done = false;
      m_ready = true;
    }
    m_cv.notify_all();
  }

  void wait()
  {
    std::unique_lock<std::mutex> lk(m_mtx);
    m_cv.wait(lk, [this]{return m_done;});
  }

private:
  std::mt19937 m_twister;
  std::mutex m_mtx;
  std::condition_variable m_cv;
  bool m_ready = false;
  bool m_done = false;
  std::function<void()> m_f;
  std::thread m_thread;
  bool m_stop = false;
};

struct named_test
{
  template<class F>
  named_test(std::string name, F&& f) : m_name(name) { s_register[name] = f; }
  ~named_test() { s_register.erase(m_name); }

  std::string m_name;
  static std::map<std::string, std::function<bool()>> s_register;
};

std::map<std::string, std::function<bool()>> named_test::s_register;
static int s_iterations = 0x7FFFFFFF;

struct test
{
  virtual ~test() {}

  virtual void reset() = 0;
  virtual void op1() = 0;
  virtual void op2() = 0;
  virtual bool eval() = 0;
};

template<class T>
bool run(T t)
{
  repetitive_task t1([&t]() { t.op1(); });
  repetitive_task t2([&t]() { t.op2(); });

  for (int i = 0; i < s_iterations; i++)
  {
    t.reset();
    t1.go();
    t2.go();
    t1.wait();
    t2.wait();
    if (not t.eval())
    {
      std::cerr << "Failure on iteration #" << i << std::endl;
      return false;
    }
  }
  return true;
};

#define REGISTER_TEST(impl) named_test s_##impl(#impl, []() { return run(impl()); });

struct store_load : test
{
  void reset() override
  {
    X = 0;
    Y = 0;
    r1 = 1;
    r2 = 1;
  }
  void op1() override
  {
    X = 1;
    r1 = Y;
  }
  void op2() override
  {
    Y = 1;
    r2 = X;
  }
  bool eval() override
  {
    return not (r1 == 0 and r2 == 0);
  }
  int X, Y;
  int r1, r2;
};
REGISTER_TEST(store_load);

struct store_load_safe : test
{
  void reset() override
  {
    X = 0;
    Y = 0;
    r1 = 1;
    r2 = 1;
  }
  void op1() override
  {
    X = 1;
    r1 = Y;
  }
  void op2() override
  {
    Y = 1;
    r2 = X;
  }
  bool eval() override
  {
    return not (r1 == 0 and r2 == 0);
  }
  std::atomic<int> X, Y;
  int r1, r2;
};
REGISTER_TEST(store_load_safe);

struct load_store : test
{
  void reset() override
  {
    X = 0;
    Y = 0;
    r1 = 0;
    r2 = 0;
  }
  void op1() override
  {
    r1 = Y;
    X = 1;
  }
  void op2() override
  {
    r2 = X;
    Y = 1;
  }
  bool eval() override
  {
    return not (r1 == 1 and r2 == 1);
  }
  int X;
  char padding1[128];
  int Y;
  char padding2[128];
  int r1;
  char padding3[128];
  int r2;
};
REGISTER_TEST(load_store);


struct mp : test
{
  void reset() override
  {
    X = 0;
    Y = 0;
    r1 = 0;
    r2 = 0;
  }
  void op1() override
  {
    X = 1;
    Y = 1;
  }
  void op2() override
  {
    r1 = Y;
    r2 = X;
  }
  bool eval() override
  {
    std::cout << "r1=" << r1 << " r2=" << r2 << "\n";
    return not (r1 == 1 and r2 == 0);
  }
  int X;
  char padding1[4096];
  int Y;
  char padding2[4096];
  int r1;
  char padding3[4096];
  int r2;
};
REGISTER_TEST(mp);


int main(int argc, const char** argv)
{
  const char* prog_name = argv[0];
  auto usage = [&]() {
    std::cerr << "Usage: " << prog_name << " [option*] [test*]\n"
              << "Options are:\n"
              << "  -h|--help       Display this help message\n"
              << "  -i|--iterations Number of iterations to perform per test\n"
              << "  -a|--all        Run all tests\n"
              << "Tests are:\n";
    for (const auto& e : named_test::s_register)
      std::cout << "  " << e.first << "\n";
    return 0;
  };
  auto runtest = [&](auto&& e) {
    std::cerr << "Running " << e.first << "...\n";
    bool success = e.second();
    if (success)
      std::cerr << "\e[32m[PASS]  " << e.first << "\e[39m\n";
    else
      std::cerr << "\e[31m[FAIL]  " << e.first << "\e[39m\n";
    return success ? 0 : -1;
  };
  if (argc <= 1) return usage();
  int ret = 0;
  while (++argv, --argc)
  {
    if (not std::strcmp(argv[0], "-h") or not std::strcmp(argv[0], "--help"))
    {
      return usage();
    }
    else if (not std::strcmp(argv[0], "-i") or not std::strcmp(argv[0], "--iterations"))
    {
      if (not (++argv, --argc)) return usage();
      s_iterations = std::atoi(argv[0]);
    }
    else if (not std::strcmp(argv[0], "-a") or not std::strcmp(argv[0], "--all"))
    {
      for (auto& e : named_test::s_register)
        ret |= runtest(e);
    }
    else
    {
      auto it = named_test::s_register.find(argv[0]);
      if (it == named_test::s_register.end()) return usage();
      ret |= runtest(*it);
    }
  }
  return ret;
}


struct contended_int : test
{
  void reset() override
  {
    t0 = std::chrono::steady_clock::now();
    X = 0;
  }
#pragma GCC push_options
#pragma GCC optimize ("O0")
  void op1() override
  {
    for (int i = 0; i < 10000000; i++)
    {
      X++;
    }
  }
  void op2() override
  {
    for (int i = 0; i < 10000000; i++)
    {
      X--;
    }
  }
#pragma GCC pop_options
  bool eval() override
  {
    auto t1 = std::chrono::steady_clock::now();
    std::cout << "It took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms.\n";
    return X == 0;
  }
  std::chrono::steady_clock::time_point t0;
  int X;
};
REGISTER_TEST(contended_int);

struct uncontended_int : test
{
  void reset() override
  {
    t0 = std::chrono::steady_clock::now();
    X = 0;
    Y = 0;
  }
#pragma GCC push_options
#pragma GCC optimize ("O0")
  void op1() override
  {
    for (int i = 0; i < 10000000; i++)
    {
      X++;
    }
  }
  void op2() override
  {
    for (int i = 0; i < 10000000; i++)
    {
      Y++;
    }
  }
#pragma GCC pop_options
  bool eval() override
  {
    auto t1 = std::chrono::steady_clock::now();
    std::cout << "It took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms.\n";
    return X == 10000000 and Y == 10000000;
  }
  std::chrono::steady_clock::time_point t0;
  int X;
  char padding[256];
  int Y;

};
REGISTER_TEST(uncontended_int);


struct contended_atomic : test
{
  void reset() override
  {
    t0 = std::chrono::steady_clock::now();
    X = 0;
  }
  void op1() override
  {
    for (int i = 0; i < 10000000; i++)
    {
      X.fetch_add(1, std::memory_order_relaxed);
    }
  }
  void op2() override
  {
    for (int i = 0; i < 10000000; i++)
    {
      X.fetch_add(-1, std::memory_order_relaxed);
    }
  }
  bool eval() override
  {
    auto t1 = std::chrono::steady_clock::now();
    std::cout << "It took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms.\n";
    return X.load() == 0;
  }
  std::chrono::steady_clock::time_point t0;
  std::atomic<int> X;
};
REGISTER_TEST(contended_atomic);

struct uncontended_atomic : test
{
  void reset() override
  {
    t0 = std::chrono::steady_clock::now();
    X = 0;
    Y = 0;
  }
  void op1() override
  {
    for (int i = 0; i < 10000000; i++)
    {
      X.fetch_add(1, std::memory_order_relaxed);
    }
  }
  void op2() override
  {
    for (int i = 0; i < 10000000; i++)
    {
      Y.fetch_add(1, std::memory_order_relaxed);
    }
  }
  bool eval() override
  {
    auto t1 = std::chrono::steady_clock::now();
    std::cout << "It took " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count() << " ms.\n";
    return X.load() == 10000000 and Y.load() == 10000000;
  }
  std::chrono::steady_clock::time_point t0;
  std::atomic<int> X;
  char padding[256];
  std::atomic<int> Y;

};
REGISTER_TEST(uncontended_atomic);

