#pragma once
 
#include <memory>

#include <re2/re2.h>

namespace varlisp {
typedef std::shared_ptr<RE2> regex_t;
// using regex_t = std::shared_ptr<RE2>;
} // namespace varlisp
