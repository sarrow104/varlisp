#include <set>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "environment.hpp"
#include "cast_visitor.hpp"
#include "eval_visitor.hpp"
#include "keyword_t.hpp"
#include "detail/buitin_info_t.hpp"
#include "tokenizer.hpp"
#include "keyword_t.hpp"
#include "detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("undef", 1, 1, eval_undef, "(undef symbol) -> boolean");

/**
 * @brief
 *      (undef symbol) -> boolean
 *      删除变量
 *      Builtin不允许删除
 *      但是，你可以在内层函数中，定义自己的操作，以修改定义；
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_undef(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "undef";
    const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&detail::car(args));
    if (!p_sym) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a symbol)");
    }
    if (varlisp::keywords_t::is_keyword(p_sym->name())) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": cannot undef keywords", p_sym->name(), ")");
    }
    bool ret = false;
    Object* it = env.find(p_sym->name());
    if (it) {
        const varlisp::Builtin * p_b = boost::get<varlisp::Builtin>(&(*it));
        if (p_b) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": cannot undef builtin function)");
        }
        env.erase(p_sym->name());
        ret = true;
    }
    return ret;
}

REGIST_BUILTIN("ifdef", 1, 1, eval_ifdef, "(ifdef symbol) -> boolean");

/**
 * @brief
 *      (ifdef symbol) -> boolean
 *      测试是否定义了某变量
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_ifdef(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "ifdef";
    const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&detail::car(args));
    if (!p_sym) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a symbol)");
    }
    if (varlisp::keywords_t::is_keyword(p_sym->name())) {
        return true;
    }
    Object* it = env.find(p_sym->name());
    return bool(it);
}

REGIST_BUILTIN("var-list", 0, 1, eval_var_list,
               "(var-list) -> int64_t\n"
               "(var-list env-name) -> int64_t ; TODO");

/**
 * @brief
 *      (var-list) -> int64_t
 *      枚举所有变量；并返回对象计数
 *      (var-list env-name) -> int64_t # TODO
 *      枚举提供的环境名所拥有的变量；并返回对象计数
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_var_list(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "ifdef";
    int64_t var_count = 0;
    if (args.length()) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": list-environment not implement)");
        // NOTE TODO 只枚举环境本身！
        // 这里需要判断，是否是关键字；
    }
    else {
        // 被子环境覆盖了的夫环境变量，不会输出。
        // keyword不允许覆盖定义
        std::set<std::string> outted;
        for (varlisp::Environment * p_env = &env; p_env; p_env = p_env->parent()) {
            for (auto it = p_env->begin(); it != p_env->end(); ++it) {
                if (outted.find(it->first) == outted.end()) {
                    std::cout << it->first << "\n"
                        << "\t" << it->second
                        << std::endl;
                    outted.insert(it->first);
                }
            }
        }
        var_count = outted.size();
    }
    return var_count;
}

REGIST_BUILTIN("let", 2, -1, eval_let,
               "(let ((symbol expr)...) (expr)...) -> result-of-last-expr");

/**
 * @brief
 *      (let ((symbol expr)...)
 *           (expr)...) -> result-of-last-expr
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_let(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "let";

    const varlisp::List * p_sym_pairs = boost::get<varlisp::List>(&detail::car(args));
    COLOG_DEBUG(detail::car(args));
    if (!p_sym_pairs) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a list; but",
                           detail::car(args), ")");
    }
    varlisp::Environment inner(&env);
    for (auto it_pair = p_sym_pairs->begin(); it_pair != p_sym_pairs->end(); ++it_pair) {
        COLOG_DEBUG(*it_pair);
        const varlisp::List* p_sym_pair =
            boost::get<varlisp::List>(&(*it_pair));
        if (!p_sym_pair) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": must be name-value  list; but",
                               p_sym_pair->nth(0), ")");
        }
        if (p_sym_pair->length() != 2) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": must be name-value  list; but",
                               p_sym_pair->nth(0), ")");
        }
        const varlisp::symbol* p_sym =
            boost::get<varlisp::symbol>(&p_sym_pair->nth(0));

        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": must be name-value  list; but",
                               *it_pair, ")");
        }
        Object value;
        // NOTE let 貌似可以达到swap两个变量值的效果；
        // 在于 getAtomicValue()的第一个参数是env，而不是inner
        // (let ((a b) (b a)) ...)
        inner[p_sym->name()] = varlisp::getAtomicValue(env,
                                                       p_sym_pair->nth(1),
                                                       value);
    }
    Object result;
    for (auto it_expr = args.begin() + 1; it_expr != args.end(); ++it_expr) {
        result = boost::apply_visitor(eval_visitor(inner), *it_expr);
    }
    
    return result;
}

REGIST_BUILTIN("setq", 2, -1, eval_setq,
               "; 对符号引用到的变量做修改；如果变量不存在，则报错\n"
               "((setq symbol1 expr1 symbol2 expr2 ... ) -> value-of-last-expr");

/**
 * @brief
 *      (setq symbol1 expr1 symbol2 expr2 ... ) -> value-of-last-expr
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_setq(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "setq";
    if (args.size() % 2 != 0) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": num of args not correct!", args.size(), ")");
    }
    Object * p_value = 0;
    for (size_t i = 0; i < args.size(); i += 2) {
        // NOTE 这里需要的是一个将 Object cast 为 symbol的函数
        Object tmpObj;
        const varlisp::symbol * p_sym = varlisp::getSymbol(env,
                                                           args.nth(i),
                                                           tmpObj);
        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": 1st must be a symbol; but ", args.nth(i),
                               ")");
        }
        p_value = env.find(p_sym->name());
        if (!p_value) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName, ": symbol, ",
                               *p_sym, " not exist!)");
        }
        Object res;
        *p_value = varlisp::getAtomicValue(env, args.nth(i + 1), res);
    }
    if (!p_value) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": last value nullptr error!)");
    }
    return *p_value;
}

REGIST_BUILTIN("setf", 2, 2, eval_setf,
               "; setf 是使用了setq的宏；暂时不支持！\n"
               "(setf \"varname\" expr) -> nil");

/**
 * @brief
 *      (setf "varname" expr) -> nil
 *
 * http://www.lispworks.com/documentation/HyperSpec/Body/m_setf_.htm
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_setf(varlisp::Environment& env, const varlisp::List& args)
{
    // TODO FIXME
    const char * funcName = "setf";
    SSS_POSITION_THROW(std::runtime_error,
                       "(",funcName,": not support yet!)");

    return varlisp::Nill{};
}

REGIST_BUILTIN("swap", 2, 2, eval_swap,
               "; swap 交换两个变量的值；变量查找办法同setq\n"
               "(swap var1 var2) -> nil");

/**
 * @brief
 *      (swap var1 var2) -> nil
 *
 * http://www.lispworks.com/documentation/HyperSpec/Body/m_setf_.htm
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_swap(varlisp::Environment& env, const varlisp::List& args)
{
    // TODO FIXME
    const char * funcName = "swap";
    const varlisp::symbol * p_sym1 = boost::get<varlisp::symbol>(&detail::car(args));
    if (!p_sym1) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a symbol)");
    }

    const varlisp::symbol * p_sym2 = boost::get<varlisp::symbol>(&detail::cadr(args));
    if (!p_sym2) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 2nd must be a symbol)");
    }

    Object * p_val1 = env.find(p_sym1->name());
    if (!p_val1) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st symbol, ", *p_sym1, " not exist)");
    }
    Object * p_val2 = env.find(p_sym2->name());
    if (!p_val2) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 2nd symbol, ", *p_sym2, " not exist)");
    }

    std::swap(*p_val1, *p_val2);
    return varlisp::Nill{};
}

} // namespace varlisp
