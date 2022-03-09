#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "../object.hpp"

#include "../builtin_helper.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/json.hpp"
#include "../json/parser.hpp"
#include "../json_print_visitor.hpp"

namespace varlisp {

namespace detail {
void json_object2stream(std::ostream& o, varlisp::Environment& env,
                        const Object& obj, const char* funcName, bool indent)
{
    Object tmp;
    const Object& objRef = varlisp::getAtomicValue(env, obj, tmp);
    if ((boost::get<varlisp::Environment>(&objRef) == nullptr) &&
        (boost::get<varlisp::List>(&objRef) == nullptr))
    {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": only Environment type or s-list type is valid; but ",
                           objRef, ")");
    }
    boost::apply_visitor(json_print_visitor(o, indent), objRef);
}

} // namespace detail

REGIST_BUILTIN("json-print", 1, 2, eval_json_print,
               "; json-print 用json格式，打印內建对象；\n"
               "; 注意，仅支持list和env\n"
               "(json-print obj boolean) -> nil");

Object eval_json_print(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "json-print";
    bool indent = false;
    if (args.length() >= 2) {
        std::array<Object, 1> objs;
        indent = *requireTypedValue<bool>(env, args.nth(1), objs[0], funcName,
                                          1, DEBUG_INFO);
    }
    detail::json_object2stream(std::cout, env, detail::car(args), funcName, indent);
    return Nill{};
}

REGIST_BUILTIN("json-string", 1, 2, eval_json_string,
               "; json-string 用json格式，序列化內建对象；\n"
               "; 注意，仅支持list和env\n"
               "(json-string obj boolean) -> nil");

Object eval_json_string(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "json-string";
    std::ostringstream oss;
    bool indent = false;
    if (args.length() >= 2) {
        std::array<Object, 1> objs;
        indent = *requireTypedValue<bool>(env, args.nth(1), objs[0], funcName,
                                          1, DEBUG_INFO);
    }
    detail::json_object2stream(oss, env, detail::car(args), funcName, indent);
    return string_t{oss.str()};
}

REGIST_BUILTIN("json-parse", 1, 1, eval_json_parse,
               "; json-parse 用json格式，解释字符串参数，并返回解析后的对象；\n"
               "; 如果解析成功返回list或者env；如果失败，返回nil\n"
               "(json-parse \"string\") -> list | env | nil");

Object eval_json_parse(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "json-parse";
    Object obj;
    const auto * p_s =
        requireTypedValue<string_t>(env, args.nth(0), obj, funcName, 0, DEBUG_INFO);

    return json::parse(p_s->to_string_view());
}

REGIST_BUILTIN("json-indent", 1, 1, eval_json_indent,
               "; json-indent 管理json的可读打印模式时候，缩进量\n"
               "(json-indent) -> \"current-json-indent-setting\"\n"
               "(json-indent \"string\") -> \"json-indent-setting\"");

Object eval_json_indent(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "json-indent";
    if (args.length() != 0U) {
        std::array<Object, 1> objs;
        const auto * p_indent =
            requireTypedValue<string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
        varlisp::detail::json::set_json_indent(*p_indent->gen_shared());
    }
    return string_t(varlisp::detail::json::get_json_indent());
}

} // namespace varlisp
