// src/detail/env_get_value.hpp
#pragma once

#include <iosfwd>

namespace varlisp {
namespace detail {

// TODO 实现模板版本；
// 用typetraits 来实现不同类型数据的获取；
//
// TODO OmegaOption.bak的路径，得从varlisp的初始化脚本中获取；
// 也就是，得有一个全局的，varlisp解释器，然后，通过特殊的函数，可以获取；
// 另外，这个解释器对象，还得给一定的接口，用来获取特定类型的变量；
// 比如，int64,double，和 string等；
// 以方便其他语言使用；
std::string get_value_with_default(std::string name, std::string def);

int64_t get_value_with_default(std::string name, int64_t def);

} // namespace detail
} // namespace varlisp
