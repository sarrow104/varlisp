#pragma once

// 目的，从sss::string_view中，解析出来，并构造成varlisp中的对象；
// 入口有两个，分别是env和list；
//

#include "../object.hpp"

#include <sss/string_view.hpp>

namespace varlisp {
namespace json {
Object parse(sss::string_view s);
} // namespace json
} // namespace varlisp
