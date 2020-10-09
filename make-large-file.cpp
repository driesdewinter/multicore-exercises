#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int, const char** argv) {
    int r = std::atoi(argv[1]);
    json doc;
    std::cin >> doc;
    int counter = 0;
    std::cout << "[";
    for (int i = 0; i < r; i++)
        for (const auto& e : doc)
            std::cout << (counter++ == 0 ? "" : ",") << e;
    std::cout << "]";
    return 0;
}
