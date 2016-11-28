#include <set>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "environment.hpp"
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

} // namespace varlisp

