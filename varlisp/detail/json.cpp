#include "json.hpp"

#include <cctype>

namespace varlisp {
namespace detail {
namespace json {

std::string& get_json_indent()
{
    static std::string indent = "  ";
    return indent;
}

void set_json_indent(const std::string& indent)
{
    size_t space_cnt = 0;
    while (space_cnt < indent.size() && std::isspace(indent[space_cnt])) {
        ++space_cnt;
    }
    get_json_indent().assign(indent, 0, space_cnt);
}

} // namespace json
} // namespace detail
} // namespace varlisp


