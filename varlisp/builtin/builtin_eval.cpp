#include "../object.hpp"

#include "../builtin_helper.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("eval", 1, 2, eval_eval, "(eval '(...)) -> ...");

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
    // const char * funcName = "eval";
    Object obj;
    const Object& refObj = varlisp::getAtomicValue(env, detail::car(args), obj);
    if (const varlisp::List* p_list = boost::get<varlisp::List>(&refObj)) {
        List expr = p_list->tail();
        Object inner;
        return varlisp::getAtomicValue(env, detail::car(expr), inner);
    }
    else {
        return refObj;
    }
    // NOTE 对于(eval (quote atomic))，返回atomic

    // SSS_POSITION_THROW(std::runtime_error,
    //                    "(", funcName, ": need squote-List as 1st argument)");
}

REGIST_BUILTIN("eval-string", 1, 2, eval_eval_string, "(eval-string \"(+ 1 2)\") -> ...");

Object eval_eval_string(varlisp::Environment& env, const varlisp::List& args)
{
    SSS_POSITION_THROW(std::runtime_error, "TODO");
}
}  // namespace varlisp
