#include <array>

#include <sss/enc/base64.hpp>

#include "../builtin_helper.hpp"
#include "../object.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../environment.hpp"

namespace varlisp {
REGIST_BUILTIN("en-base64", 1, 1, eval_en_base64,
               "(en-base64 \"string\") -> \"enc\"");

Object eval_en_base64(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "en-base64";
    std::array<Object, 1> objs;
    auto * p_str =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    sss::enc::Base64 b;
    return varlisp::string_t(b.encode(p_str->to_string()));
}

REGIST_BUILTIN("de-base64", 1, 1, eval_de_base64,
               "(de-base64 \"string\") -> \"decode\"");

Object eval_de_base64(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "de-base64";
    std::array<Object, 1> objs;
    auto * p_str =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    sss::enc::Base64 b;
    return varlisp::string_t(b.decode(p_str->to_string()));
}

} // namespace varlisp
