// src/detail/env_get_value.cpp
#include "env_get_value.hpp"

#include "../interpreter.hpp"
#include "../builtin_helper.hpp"

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
std::string get_value_with_default(std::string name, std::string def)
{
    auto& env = varlisp::Interpreter::get_instance().get_env();
    auto* objPtr = env.deep_find(name);
    std::string res = def;

    if (objPtr) {
        Object objTmp;

        const string_t* p_str = getTypedValue<varlisp::string_t>(env, *objPtr, objTmp);

        if (p_str) {
            p_str->copy_to_string(res);
        }
    }
    return res;
}

int64_t get_value_with_default(std::string name, int64_t def)
{
    auto& env = varlisp::Interpreter::get_instance().get_env();
    auto* objPtr = env.deep_find(name);
    int64_t res = def;

    if (objPtr) {
        Object objTmp;

        const int64_t* p_res = getTypedValue<int64_t>(env, *objPtr, objTmp);

        if (p_res) {
            res = *p_res;
        }
    }
    return res;
}


} // namespace detail
} // namespace varlisp

