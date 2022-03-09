#include <string>

namespace varlisp::detail::json {

std::string& get_json_indent(); 
void set_json_indent(const std::string& indent); 

} // namespace varlisp::detail::json

