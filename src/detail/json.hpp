#include <string>

namespace varlisp {
namespace detail {
namespace json {

std::string& get_json_indent(); 
void set_json_indent(const std::string& indent); 

} // namespace json
} // namespace detail
} // namespace varlisp

