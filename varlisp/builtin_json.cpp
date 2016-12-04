#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "object.hpp"

#include "builtin_helper.hpp"
#include "detail/buitin_info_t.hpp"
#include "json_print_visitor.hpp"
#include "json/parser.hpp"

namespace varlisp {

REGIST_BUILTIN("json-print", 1, 1, eval_json_print,
               "; json-print 用json格式，打印內建对象；\n"
               "; 注意，仅支持list和env\n"
               "(json-print obj) -> nil");

Object eval_json_print(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "json-print";
    Object tmp;
    const Object& objRef = varlisp::getAtomicValue(env, args.head, tmp);
    if (boost::get<varlisp::Environment>(&objRef)) {
        // OK
    }
    else if (const varlisp::List* p_l = boost::get<varlisp::List>(&objRef)) {
        if (!p_l->is_squote()) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": only s-list is valid; but ",
                               objRef, ")");
        }
        // OK
    }
    else {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": only Environment type or s-list type is valid; but ",
                           objRef, ")");
    }
    boost::apply_visitor(json_print_visitor(std::cout), objRef);
    return Nill{};
}

REGIST_BUILTIN("json-parse", 1, 1, eval_json_parse,
               "; json-parse 用json格式，解释字符串参数，并返回解析后的对象；\n"
               "; 如果解析成功返回list或者env；如果失败，返回nil\n"
               "; TODO complete me!\n"
               "(json-parse \"string\") -> list | env | nil");

Object eval_json_parse(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "json-parse";
    // 这基本需要重写解析器了
    Object obj;
    const string_t * p_s = varlisp::getTypedValue<varlisp::string_t>(env, args.head, obj);
    if (!p_s) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st argument must be string; but ",
                           args.head, ")");
    }

    return json::parse(p_s->to_string_view());
}

} // namespace varlisp
