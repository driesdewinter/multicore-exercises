#include "fifo.h"
#include <iostream>
#include <variant>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>
#include <sys/prctl.h>

using json = nlohmann::json;

struct indent { std::size_t level; };
std::ostream& operator<<(std::ostream& os, indent value) {
    while (value.level--)
        os << "    ";
    return os;
}

struct json_as_xml : nlohmann::json_sax<json> {
    struct begin_group_t {};
    struct end_group_t {};
    struct key_t { std::string s; };
    struct done_t {};

    using any_t = std::variant<const char*,
                               number_integer_t,
                               number_unsigned_t,
                               number_float_t,
                               string_t,
                               begin_group_t,
                               end_group_t,
                               key_t,
                               done_t>;

    std::vector<std::string> stack;

    fifo<any_t> queue;
    std::thread thread;
    bool done = false;

    json_as_xml() {
        thread = std::thread([this]() {
            prctl(PR_SET_NAME, "formatter", nullptr, nullptr, nullptr);
            while (not done)
                dump(queue.pop());
        });
    }

    ~json_as_xml() {
        post(done_t{}); // signal that parsing is done;
        thread.join();
    }

    const char* current_tag() const {
        if (stack.empty())        return "doc";
        if (stack.back().empty()) return "item";
        else                      return stack.back().c_str();
    }

    template<class T>
    void dump(T&& value) {
        std::cout << indent{ stack.size() } << "<" << current_tag() << ">" << value << "</" << current_tag() << ">\n";
    }

    void dump(string_t&& value) {
        std::cout << indent{ stack.size() } << "<" << current_tag() << ">";
        for (char c : value) {
            switch (c) {
            case '<': std::cout << "&lt;" ; break;
            case '>': std::cout << "&gt;" ; break;
            case '&': std::cout << "&amp;"; break;
            default:
                std::cout << c;
                break;
            }
        }
        std::cout << "</" << current_tag() << ">\n";
    }

    void dump(begin_group_t&&) {
        std::cout << indent{ stack.size() } << "<" << current_tag() << ">\n";
        stack.emplace_back();
    }

    void dump(end_group_t&&) {
        stack.pop_back();
        std::cout << indent{ stack.size() } << "</" << current_tag() << ">\n";
    }

    void dump(key_t&& value) {
        stack.back() = std::move(value.s);
    }

    void dump(done_t&&) {
        done = true;
    }

    void dump(any_t&& value) {
        // This function is called upon every SAX parser call
        // Demux to the right dump()-overload.
        std::visit([&](auto&& arg) {
            dump(std::move(arg));
        }, std::move(value));
    }

    bool post(any_t&& value) {
        // This function is called upon every SAX parser call.
        queue.push(std::move(value));
        return true;
    }

    bool null()                                            { return post(""); }
    bool boolean(bool val)                                 { return post(val ? "true" : "false"); }
    bool number_integer(number_integer_t val)              { return post(val); }
    bool number_unsigned(number_unsigned_t val)            { return post(val); }
    bool number_float(number_float_t val, const string_t&) { return post(val); }
    bool string(string_t& val)                             { return post(std::move(val)); }
    bool binary(binary_t&)                                 { return true; }

    bool start_object(std::size_t) { return post(begin_group_t{}); }
    bool end_object()              { return post(end_group_t{}); }
    bool start_array(std::size_t)  { return post(begin_group_t{}); }
    bool end_array()               { return post(end_group_t{}); }

    bool key(string_t& val) { return post(key_t{std::move(val)}); }

    bool parse_error(std::size_t, const std::string&, const nlohmann::detail::exception&) { return true; }
};

int main() {
    prctl(PR_SET_NAME, "parser", nullptr, nullptr, nullptr);
    std::ios::sync_with_stdio(false);
    std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    json_as_xml doc;
    json::sax_parse(std::cin, &doc);
    return 0;
}
