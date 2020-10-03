#include "fifo.h"
#include <iostream>
#include <variant>
#include <nlohmann/json.hpp>
#include <thread>
#include <vector>
#include <sys/prctl.h>
#include <sstream>

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

    using input_t = std::variant<const char*,
                                 number_integer_t,
                                 number_unsigned_t,
                                 number_float_t,
                                 string_t,
                                 begin_group_t,
                                 end_group_t,
                                 key_t,
                                 done_t>;

    using output_t = std::variant<std::string, done_t>;

    struct formatter {
        std::vector<std::string> stack = { "" };
        std::thread thread;
        fifo<input_t> input;
        fifo<output_t> output;
        std::ostringstream os;
        bool formatter_done = false;
        bool writer_done = false;

        formatter() {
            thread = std::thread([this] {
                prctl(PR_SET_NAME, "formatter", nullptr, nullptr, nullptr);
                while (not formatter_done)
                    dump(input.pop());
            });
        }

        ~formatter() {
            thread.join();
        }

        const char* current_tag() const {
            if (stack.back().empty()) return "item";
            else                      return stack.back().c_str();
        }

        template<class T>
        void dump(T&& value) {
            os << indent{ stack.size() }<< "<" << current_tag() << ">" << value << "</" << current_tag() << ">\n";
        }

        void dump(string_t&& value) {
            os << indent{ stack.size() } << "<" << current_tag() << ">";
            for (char c : value)
            {
                switch (c)
                {
                case '<': os << "&lt;" ; break;
                case '>': os << "&gt;" ; break;
                case '&': os << "&amp;"; break;
                default:
                    os << c;
                    break;
                }
            }
            os << "</" << current_tag() << ">\n";
        }

        void dump(begin_group_t&&) {
            os << indent{ stack.size() } << "<" << current_tag() << ">\n";
            stack.emplace_back();
        }

        void dump(end_group_t&&) {
            stack.pop_back();
            os << indent{ stack.size() } << "</" << current_tag() << ">\n";
            if (stack.size() == 1) {
                output.push(os.str());
                os.str("");
            }
        }

        void dump(key_t&& value) {
            stack.back() = std::move(value.s);
        }

        void dump(done_t&&) {
            formatter_done = true;
            output.push(done_t{});
        }

        void dump(input_t&& value) {
            std::visit([&](auto&& arg) {
                dump(std::move(arg));
            }, std::move(value));
        }
    };
    std::vector<std::unique_ptr<formatter>> formatters;
    std::thread writer;

    json_as_xml() {
        formatters.resize(2);
        for (auto& f : formatters)
            f = std::make_unique<formatter>();

        writer = std::thread([this] {
            prctl(PR_SET_NAME, "writer", nullptr, nullptr, nullptr);
            while (not std::all_of(formatters.begin(), formatters.end(),
                    [&](const std::unique_ptr<formatter>& f) {return f->writer_done;})) {
                for (auto& f : formatters) {
                    if (f->writer_done)
                        continue;
                    dump(*f, f->output.pop());
                }
            }
        });
        std::cout << "<doc>\n";
    }

    ~json_as_xml() {
        for (auto& f : formatters)
            f->input.push(done_t {}); // signal that parsing is done;
        writer.join();
        std::cout << "</doc>\n";
    }

    void dump(formatter&, std::string&& s) {
        std::cout << s;
    }

    void dump(formatter& f, done_t&&) {
        f.writer_done = true;
    }

    void dump(formatter& f, output_t&& value) {
        std::visit([&](auto&& arg) {dump(f, std::move(arg));}, std::move(value));
    }

    unsigned int formatter_index = 0;
    unsigned int stack_depth = 0;

    bool post(input_t&& value) {
        formatters[formatter_index]->input.push(std::move(value));
        return true;
    }

    bool begin_group() {
        if (stack_depth > 0)
            post(begin_group_t {});
        stack_depth++;
        return true;
    }

    bool end_group() {
        stack_depth--;
        if (stack_depth > 0)
            post(end_group_t {});
        if (stack_depth == 1)
            formatter_index = (formatter_index + 1) % formatters.size();
        return true;
    }

    bool null()                                            { return post(""); }
    bool boolean(bool val)                                 { return post(val ? "true" : "false"); }
    bool number_integer(number_integer_t val)              { return post(val); }
    bool number_unsigned(number_unsigned_t val)            { return post(val); }
    bool number_float(number_float_t val, const string_t&) { return post(val); }
    bool string(string_t& val)                             { return post(std::move(val)); }
    bool binary(binary_t&)                                 { return true; }

    bool start_object(std::size_t) { return begin_group(); }
    bool end_object()              { return end_group(); }
    bool start_array(std::size_t)  { return begin_group(); }
    bool end_array()               { return end_group(); }

    bool key(string_t& val) { return post(key_t{std::move(val)}); }

    bool parse_error(std::size_t, const std::string&, const nlohmann::detail::exception&) {return true;}
};

int main() {
    prctl(PR_SET_NAME, "parser", nullptr, nullptr, nullptr);
    std::ios::sync_with_stdio(false);
    std::cout << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    {
        json_as_xml doc;
        json::sax_parse(std::cin, &doc);
    }
    return 0;
}
