#include <iostream>
#include <nlohmann/json.hpp>

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
        for (const auto& [key, val] : doc.j.items()) {
            os << "\n" << indent{ doc.level + 1 } << "<item>" << json_as_xml{ val, doc.level + 1 } << "</item>";
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
