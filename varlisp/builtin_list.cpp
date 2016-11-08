#include "builtin_helper.hpp"
#include "eval_visitor.hpp"
#include "object.hpp"

#include <sss/util/PostionThrow.hpp>
#include <stdexcept>

namespace varlisp {
/**
 * @brief (car (list item1 item2 ...)) -> item1
 *
 * @param env
 * @param args
 *
 * @return
 */
Object eval_car(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args, obj);
    if (!p_list) {
        SSS_POSTION_THROW(std::runtime_error, "(car: need squote-List)");
    }

    return p_list->car();
}

/**
 * @brief (cdr '(list item1 item2 ...)) -> '(item2 item3 ...)
 *
 * @param env
 * @param args
 *
 * @return
 *
 * TODO FIXME 这个函数，需要类似eval_car改造。
 */
Object eval_cdr(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args, obj);
    if (!p_list) {
        SSS_POSTION_THROW(std::runtime_error, "(cdr: need squote-List)");
    }
    return p_list->cdr();
}

}  // namespace varlisp
