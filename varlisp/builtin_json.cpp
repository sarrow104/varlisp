#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "object.hpp"

#include "builtin_helper.hpp"
#include "detail/buitin_info_t.hpp"
#include "json_print_visitor.hpp"

namespace varlisp {

REGIST_BUILTIN("json-print", 1, 1, eval_json_print,
               "(json-print obj)");

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

} // namespace varlisp
