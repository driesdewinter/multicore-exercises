#ifndef TEST_MAIN_H_
#define TEST_MAIN_H_

#include <map>
#include <string>
#include <functional>
#include <iostream>
#include <cstring>

struct named_test
{
    template<class F>
    named_test(std::string name, F&& f) : m_name(name) { s_register[name] = f; }
    ~named_test() { s_register.erase(m_name); }

    std::string m_name;
    static std::map<std::string, std::function<bool()>> s_register;
};

std::map<std::string, std::function<bool()>> named_test::s_register;
int s_iterations = 100000000;

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

#endif /* TEST_MAIN_H_ */
