#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>
#include <sstream>
#include <chrono>

using json = nlohmann::json;

struct indent { std::size_t level; };
std::ostream& operator<<(std::ostream& os, indent value) {
    while (value.level--)
        os << "    ";
    return os;
}

struct json_as_xml {
    const json& j;
    std::size_t level = 0;
};
std::ostream& operator<<(std::ostream& os, json_as_xml doc) {
    switch (doc.j.type()) {
    case json::value_t::array:
        if (doc.level == 0) {
            auto t0 = std::chrono::steady_clock::now();

            std::size_t number_of_threads = std::thread::hardware_concurrency();
            std::size_t items_per_thread = (doc.j.size() + number_of_threads - 1) / number_of_threads;
            std::vector<std::thread> threads;
            std::vector<std::ostringstream> oss(number_of_threads);

            for (std::size_t i = 0; i < number_of_threads; i++) {
                threads.emplace_back([&, &os = oss[i], i] {
                    auto tt0 = std::chrono::steady_clock::now();
                    for (std::size_t j = i * items_per_thread; j < std::min((i + 1) * items_per_thread, doc.j.size()); j++)
                    {
                        os << "\n" << indent{doc.level + 1}<< "<item>"
                           << json_as_xml{doc.j[j], doc.level + 1}
                           << "</item>";
                    }
                    auto tt1 = std::chrono::steady_clock::now();
                    std::cerr << "Thread took " << (tt1 - tt0).count() << "ns.\n";
                });
            }

            auto t1 = std::chrono::steady_clock::now();

            for (std::size_t i = 0; i < number_of_threads; i++) {
                threads[i].join();
            }
            auto t2 = std::chrono::steady_clock::now();

            for (std::size_t i = 0; i < number_of_threads; i++) {
                os << oss[i].str();
            }
            auto t3 = std::chrono::steady_clock::now();

            std::cerr << "Root array took " << (t1 - t0).count() << "ns + " << (t2 - t1).count() << "ns + "
                      << (t3 - t2).count() << "ns.\n";
        } else {
            for (const auto& [key, val] : doc.j.items()) {
                os << "\n" << indent{ doc.level + 1 } << "<item>" << json_as_xml{ val, doc.level + 1 } << "</item>";
            }
        }
        return os << "\n" << indent{ doc.level };
    case json::value_t::object:
        for (const auto& [key, val] : doc.j.items()) {
            os << "\n" << indent{ doc.level + 1 } << "<" << key << ">" << json_as_xml{ val, doc.level + 1 } << "</"
                    << key << ">";
        }
        return os << "\n" << indent{ doc.level };

    case json::value_t::string:
        for (char c : doc.j.get<std::string>()) {
            switch (c) {
            case '<': os << "&lt;" ; break;
            case '>': os << "&gt;" ; break;
            case '&': os << "&amp;"; break;
            default:
                os << c;
                break;
            }
        }
        return os;

    case json::value_t::boolean:
        return os << (doc.j.get<bool>() ? "true" : "false");

    case json::value_t::number_integer:
        return os << doc.j.get<int64_t>();

    case json::value_t::number_unsigned:
        return os << doc.j.get<uint64_t>();

    case json::value_t::number_float:
        return os << doc.j.get<double>();

    case json::value_t::null:
    default:
        return os;
    }
}

int main() {
    auto t0 = std::chrono::steady_clock::now();
    auto t1 = t0;
    auto t2 = t1;
    {
        json doc;
        std::cin >> doc;
        t1 = std::chrono::steady_clock::now();
        std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" << "<doc>" << json_as_xml{ doc } << "</doc>\n";
        t2 = std::chrono::steady_clock::now();
    }
    auto t3 = std::chrono::steady_clock::now();
    std::cerr << "It took " << (t1 - t0).count() << "ns + " << (t2 - t1).count() << "ns + " << (t3 - t2).count()
            << "ns.\n";
    return 0;
}
