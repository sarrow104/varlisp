#include "object.hpp"

#include "builtin_helper.hpp"
#include "detail/buitin_info_t.hpp"

namespace varlisp {

/**
 * @brief (eval '(...)) -> ...
 *
 * NOTE DrRacket中 (eval ...) 支持 1 到 2 个参数。
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_eval(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "eval";
    Object obj;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args, obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need squote-List as 1st argument)");
    }
    return p_list->next()->eval(env);
}

REGIST_BUILTIN("eval", 1, 2, eval_eval, "(eval '(...)) -> ...");

}  // namespace varlisp
