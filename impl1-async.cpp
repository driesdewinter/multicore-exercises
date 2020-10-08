#include <iostream>
#include <nlohmann/json.hpp>
#include <future>
#include <sstream>
#include <mutex>
#include <boost/asio.hpp>
#include <optional>

namespace {
boost::asio::io_context io_context;
boost::asio::executor_work_guard<boost::asio::io_context::executor_type> io_work_guard(io_context.get_executor());
std::vector<std::thread> io_threads;
std::once_flag io_started;
struct io_demolition {
    ~io_demolition() {
        io_work_guard.reset();
        for (auto& thread : io_threads) {
            thread.join();
        }
    }
};
std::optional<io_demolition> io_demolition_guard;
}

template<class Function, class ... Args>
std::future<std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>> my_async(Function&& f, Args&& ... args) {
    using result_type = std::invoke_result_t<std::decay_t<Function>, std::decay_t<Args>...>;
    std::call_once(io_started, [&]() {
        for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++) {
            io_threads.emplace_back([]() {
                io_context.run();
            });
        }
        io_demolition_guard.emplace();
    });
    std::promise<result_type> promise;
    std::future<result_type> future = promise.get_future();

    boost::asio::post(io_context,
            [promise = std::move(promise), f = std::forward<Function>(f),
                    args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                promise.set_value(std::apply(f, std::move(args)));
            });
    return future;
}

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
            std::vector<std::future<std::stringstream>> results;

            for (const auto& [key, val] : doc.j.items()) {
                results.push_back(my_async([&]() {
                    std::stringstream ss;
                    ss << "\n" << indent{ doc.level + 1 } << "<item>" << json_as_xml{ val, doc.level + 1 } <<"</item>" ;
                    return ss;
                }));
            }

            for (auto& result : results) {
                os << result.get().str();
            }
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
    json doc;
    std::cin >> doc;
    std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" << "<doc>" << json_as_xml{ doc } << "</doc>\n";
    return 0;
}
