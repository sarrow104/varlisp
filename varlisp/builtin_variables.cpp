#include <set>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "environment.hpp"
#include "keyword_t.hpp"
#include "detail/buitin_info_t.hpp"
#include "tokenizer.hpp"
#include "keyword_t.hpp"

namespace varlisp {

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
    const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&args.head);
    if (!p_sym) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a symbol)");
    }
    if (varlisp::keywords_t::is_keyword(p_sym->m_data)) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": cannot undef keywords", p_sym->m_data, ")");
    }
    bool ret = false;
    Object* it = env.find(p_sym->m_data);
    if (it) {
        const varlisp::Builtin * p_b = boost::get<varlisp::Builtin>(&(*it));
        if (p_b) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": cannot undef builtin function)");
        }
        env.erase(p_sym->m_data);
        ret = true;
    }
    return ret;
}

REGIST_BUILTIN("undef", 1, 1, eval_undef, "(undef symbol) -> boolean");

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
    const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&args.head);
    if (!p_sym) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a symbol)");
    }
    if (varlisp::keywords_t::is_keyword(p_sym->m_data)) {
        return true;
    }
    Object* it = env.find(p_sym->m_data);
    return bool(it);
}

REGIST_BUILTIN("ifdef", 1, 1, eval_ifdef, "(ifdef symbol) -> boolean");

/**
 * @brief
 *      (var-list) -> int
 *      枚举所有变量；并返回对象计数
 *      (var-list env-name) -> int # TODO
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
    int var_count = 0;
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

REGIST_BUILTIN("var-list", 0, 1, eval_var_list,
               "(var-list) -> int\n"
               "(var-list env-name) -> int ; TODO");

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
#if 0
    const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&args.head);
    if (!p_sym) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a symbol)");
    }
    Object res;
    env[p_sym->m_data] = varlisp::getAtomicValue(env, args.tail[0].head, res);
    return varlisp::Nill{};
#else
    // NOTE let 貌似可以达到交互两个变量值的效果？
    // (let ((a b) (b a)) ...)
    const varlisp::List * p_sym_pairs = boost::get<varlisp::List>(&args.head);
    COLOG_DEBUG(args.head);
    if (!p_sym_pairs) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a list; but",
                           args.head, ")");
    }
    varlisp::Environment inner(&env);
    for (; p_sym_pairs && p_sym_pairs->head.which();
         p_sym_pairs = p_sym_pairs->next()) {
        COLOG_DEBUG(p_sym_pairs->head);
        const varlisp::List* p_sym_pair =
            boost::get<varlisp::List>(&p_sym_pairs->head);
        if (!p_sym_pair) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": must be name-value  list; but",
                               p_sym_pairs->head, ")");
        }
        if (p_sym_pair->length() != 2) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": must be name-value  list; but",
                               p_sym_pairs->head, ")");
        }
        const varlisp::symbol* p_sym =
            boost::get<varlisp::symbol>(&p_sym_pair->head);

        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": must be name-value  list; but",
                               p_sym_pairs->head, ")");
        }
        Object value;
        inner[p_sym->m_data] = varlisp::getAtomicValue(inner,
                                                       p_sym_pair->tail[0].head,
                                                       value);
    }
    Object result;
    for (const varlisp::List * p_exprs = args.next();
         p_exprs && p_exprs->head.which();
         p_exprs = p_exprs->next()) {
        result = boost::apply_visitor(eval_visitor(inner), p_exprs->head);
    }
    
    return result;
#endif
}

REGIST_BUILTIN("let", 1, -1, eval_let,
               "(let ((symbol expr)...) (expr)...) -> result-of-last-expr");

namespace detail {
bool is_symbol(const std::string& name)
{
    varlisp::Tokenizer t{name};
    auto tok = t.top();
    if (t.lookahead_nothrow(1).which()) {
        return false;
    }
    varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&tok);
    if (!p_sym) {
        return false;
    }
    if (p_sym->m_data != name) {
        return false;
    }
    if (varlisp::keywords_t::is_keyword(p_sym->m_data)) {
        return false;
    }
    return true;
}
inline bool is_car_valid(const varlisp::List * p_list)
{
    return p_list && p_list->head.which();
}
} // namespace detail

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
    const varlisp::List * p_1st = &args;
    const varlisp::List * p_2nd = p_1st->next();
    Object * p_value = 0;
    while (detail::is_car_valid(p_1st)) {
        const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&p_1st->head);
        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": 1st must be a symbol)");
        }
        p_value = env.find(p_sym->m_data);
        if (!p_value) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": symbol, ", *p_sym, " not exist!)");
        }
        if (!detail::is_car_valid(p_2nd)) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": 2nd value for ", *p_sym, " is empty! '())");
        }
        Object res;
        *p_value = varlisp::getAtomicValue(env, p_2nd->head, res);
        p_1st = p_2nd->next();
        if (p_1st) {
            p_2nd = p_1st->next();
        }
    }
    if (!p_value) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": last value nullptr error!)");
    }
    return *p_value;
}

REGIST_BUILTIN("setq", 2, -1, eval_setq,
               "; 对符号引用到的变量做修改；如果变量不存在，则报错\n"
               "((setq symbol1 expr1 symbol2 expr2 ... ) -> value-of-last-expr");

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

REGIST_BUILTIN("setf", 2, 2, eval_setf,
               "; setf 是使用了setq的宏；暂时不支持！\n"
               "(setf \"varname\" expr) -> nil");

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
    const varlisp::symbol * p_sym1 = boost::get<varlisp::symbol>(&args.head);
    if (!p_sym1) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a symbol)");
    }

    const varlisp::symbol * p_sym2 = boost::get<varlisp::symbol>(&args.tail[0].head);
    if (!p_sym2) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 2nd must be a symbol)");
    }

    Object * p_val1 = env.find(p_sym1->m_data);
    if (!p_val1) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st symbol, ", *p_sym1, " not exist)");
    }
    Object * p_val2 = env.find(p_sym2->m_data);
    if (!p_val2) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 2nd symbol, ", *p_sym2, " not exist)");
    }

    std::swap(*p_val1, *p_val2);
    return varlisp::Nill{};
}

REGIST_BUILTIN("swap", 2, 2, eval_swap,
               "; swap 交换两个变量的值；变量查找办法同setq\n"
               "(swap var1 var2) -> nil");

} // namespace varlisp
