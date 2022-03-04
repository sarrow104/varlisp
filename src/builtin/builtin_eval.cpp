#include <sss/debug/value_msg.hpp>

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
    const Object& refObj = varlisp::getAtomicValue(env, args.nth(0), obj);
    if (const varlisp::List* p_list = boost::get<varlisp::List>(&refObj)) {
        if (p_list->is_quoted()) {
            Object result;
            return varlisp::getAtomicValue(env, *p_list->unquote(), result);
        }
        else {
            return p_list->eval(env);
        }
    }
    else {
        return refObj;
    }
    // NOTE 对于(eval (quote atomic))，返回atomic
}

REGIST_BUILTIN("eval-string", 1, 2, eval_eval_string, "(eval-string \"(+ 1 2)\") -> ...");

Object eval_eval_string(varlisp::Environment& env, const varlisp::List& args)
{
    SSS_POSITION_THROW(std::runtime_error, "TODO");
    // NOTE 需要改造解析器！
    // 同时需要改造的，还有load；行为类似；
    // 区别只是load多了一个读取文件的动作；
    //
    // 另外，最好再增加一个文件作用域的return，以便中止解析；
    //
    // 另外，需要做成一句一句执行，并且返回最后一个语句的结果。
    //
    // 还有，可以考虑，传入文件级别的变量——好比命令行参数一样！
}
}  // namespace varlisp
