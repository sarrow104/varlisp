#include <stdexcept>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>

#include "../object.hpp"

#include "../builtin_helper.hpp"
#include "../strict_less_visitor.hpp"
#include "../cast2bool_visitor.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/list_iterator.hpp"

// TODO
//
// (for (a {})
//   exprs...)
// 循环遍历{}中每个object——a为object
//
// (for-map (k v {})
//   exprs...)
// k = (quote symbol)
// v = object
// 同样是循环

namespace varlisp {

REGIST_BUILTIN("for", 1, -1, eval_for,
               "; for 循环\n"
               "; 为了能在一个函数名下，容纳更多的调用方式，特如下设计——\n"
               "; 即，通过参数个数，来区分调用。\n"
               "; 需要注意的是，形式2,3，由于省略了判断语句，因此只能用于单\n"
               "; numeric变量的循环。\n"
               "1. (for (var s-list) expr...) ->\n"
               "\tresult-of-last-expr\n"
               "2. (for (var ini-value end-value) expr...) ->\n"
               "\tresult-of-last-expr\n"
               "3. (for (var ini-value end-value step) expr...) ->\n"
               "\tresult-of-last-expr\n"
               "4. (for ((v1 a1 v2 a2...) condition (iterate-expr)) expr...) ->\n"
               "\tresult-of-last-expr");

Object eval_loop_list(varlisp::Environment& env,
                      const varlisp::symbol& sym,
                      const varlisp::List slist,
                      const varlisp::List exprs);

Object eval_loop_step(varlisp::Environment& env,
                      const varlisp::symbol& sym,
                      const varlisp::Object& start,
                      const varlisp::Object& end,
                      const varlisp::Object& step,
                      const varlisp::List exprs);

Object eval_loop_condition(varlisp::Environment& env,
                           const varlisp::List* p_ctrl_block,
                           const varlisp::List exprs);

/**
 * @brief
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_for(varlisp::Environment& env, const varlisp::List& args)
{
    COLOG_DEBUG(args);
    const char * funcName = "for";
    const varlisp::List* p_loop_ctrl =
        boost::get<varlisp::List>(&detail::car(args));
    COLOG_DEBUG(detail::car(args));

    if (!p_loop_ctrl) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a list; but",
                           detail::car(args), ")");
    }
    if (p_loop_ctrl->is_quoted()) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must not be a s-list; but",
                           detail::car(args), ")");
    }
    COLOG_DEBUG(SSS_VALUE_MSG(p_loop_ctrl->is_quoted()));
    COLOG_DEBUG(SSS_VALUE_MSG(detail::car(*p_loop_ctrl)));
    int ctrl_block_len = p_loop_ctrl->length();
    COLOG_DEBUG(SSS_VALUE_MSG(ctrl_block_len));
    Object tmpSym;
    const varlisp::symbol* p_sym =
        boost::get<varlisp::symbol>(&detail::car(*p_loop_ctrl));

    if (2 == ctrl_block_len) {
        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": a symbol is_needed as iterator arg; but",
                               detail::car(*p_loop_ctrl), ")");
        }
        Object tmpSList;
        const varlisp::List* p_slist = varlisp::getQuotedList(
            env, detail::cadr(*p_loop_ctrl), tmpSList);
        if (!p_slist) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": a slist is_needed as iterator range; but",
                               detail::cadr(*p_loop_ctrl), ")");
        }
        return eval_loop_list(env, *p_sym, *p_slist, args.tail());
    }
    else if (4 == ctrl_block_len || (3 == ctrl_block_len && p_sym)) {
        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": a symbol is_needed as iterator arg; but",
                               detail::car(*p_loop_ctrl), ")");
        }
        Object tmpStart;
        Object tmpEnd;
        Object tmpStep{int64_t(1)};
        return eval_loop_step(
            env, *p_sym,
            varlisp::getAtomicValue(env, detail::cadr(*p_loop_ctrl), tmpStart),
            varlisp::getAtomicValue(env, detail::caddr(*p_loop_ctrl), tmpEnd),
            ctrl_block_len == 4 ? varlisp::getAtomicValue(
                                      env, p_loop_ctrl->nth(2), tmpStep)
                                : tmpStep,
            args.tail());
    }
    else if (3 == ctrl_block_len && !p_sym) {
        return eval_loop_condition(env, p_loop_ctrl, args.tail());
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": invalid arguments: ", args, ")");
    }
}

Object eval_loop_list(varlisp::Environment& env,
                      const varlisp::symbol& sym,
                      const varlisp::List slist,
                      const varlisp::List exprs)
{
    varlisp::Environment inner(&env);
    Object result;
    for (auto it = slist.begin(); it != slist.end(); ++it) {
        inner[sym.name()] = *it;
        for (auto expr_it = exprs.begin();
             expr_it != exprs.end();
             ++expr_it)
        {
            result = boost::apply_visitor(eval_visitor(inner), *expr_it);
        }
    }
    return result;
}

struct loop_ctrl_t
{
private:
    varlisp::Environment&          m_env;
    std::vector<std::string>       m_sym_vec;
    std::vector<varlisp::Object>   m_init_value_vec;
    const varlisp::Object &        m_condition;
    const varlisp::Object &        m_next;

public:
    loop_ctrl_t(varlisp::Environment& env,
                const std::string& sym, const Object& value,
                const varlisp::Object & condition,
                const varlisp::Object & next)
        : m_env(env),
          m_condition(condition),
          m_next(next)
    {
        m_sym_vec.push_back(sym);
        m_init_value_vec.push_back(value);
        COLOG_DEBUG(m_condition, m_next);
    }
    loop_ctrl_t(varlisp::Environment& env,
                const varlisp::List* p_ctrl_block)
        : m_env(env),
          m_condition(detail::cadr(*p_ctrl_block)),
          m_next(detail::caddr(*p_ctrl_block))
    {
        COLOG_DEBUG(m_condition, m_next);
        const char * funcName = "for";
        const varlisp::List* p_kv_pair_list =
            boost::get<varlisp::List>(&detail::car(*p_ctrl_block));
        if (!p_kv_pair_list) {
            SSS_POSITION_THROW(std::runtime_error, detail::car(*p_ctrl_block));
        }

        if (p_kv_pair_list->size() == 0 || p_kv_pair_list->size() % 2) {
            SSS_POSITION_THROW(std::runtime_error,
                               "for(v4) need positive even number elements");
        }

        COLOG_DEBUG("kv pair list lengh = ", p_kv_pair_list->size());

        for (size_t i = 0; i < p_kv_pair_list->size(); i += 2) {
            Object tmpObj;
            const varlisp::symbol * p_sym = varlisp::getSymbol(
                m_env,
                p_kv_pair_list->nth(i),
                tmpObj);
            if (!p_sym) {
                SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                                   ": 1st must be a symbol; but ", p_kv_pair_list->nth(i),
                                   ")");
            }
            m_sym_vec.push_back(p_sym->name());
            Object res;
            m_init_value_vec.push_back(getAtomicValue(env, p_kv_pair_list->nth(i + 1), res));
            COLOG_DEBUG(m_sym_vec.back(), m_init_value_vec.back());
        }
    }

    // (for ((a 1 b 10 c 1)
    //       (< a b)
    //       (setq a (+ a 1)))
    //      (format 1 "{}\n" a))
    // 
    // (for ((a 1 b 10 c 1) (< a b) (setq a (+ a 1))) (format 1 "{}\n" a))
    // (for ((a 1 b 10 c 1) (< a b) (setq a (+ a 1))) (format 1 "{} {} {}\n" a b c ))
    //
    // (for (a 1 10) (format 1 "{}" a))

    void start() {
        for (size_t i = 0; i != m_sym_vec.size(); ++i) {
            m_env[m_sym_vec[i]] = m_init_value_vec[i];
            COLOG_DEBUG("loop ", i, m_sym_vec[i], '=', m_init_value_vec[i]);
        }
    }
    bool condition() {
        Object tmpRes;
        bool is_condition = boost::apply_visitor(
            cast2bool_visitor(m_env),
            varlisp::getAtomicValue(m_env, m_condition, tmpRes));
        COLOG_DEBUG(is_condition, m_condition);
        return is_condition;
    }
    void next()
    {
        Object tmpRes;
        varlisp::getAtomicValue(m_env, m_next, tmpRes);
    }
    Object loop(Environment& env, const varlisp::List exprs)
    {
        varlisp::Environment inner(&env);
        Object result = Nill{};

        for (this->start(); this->condition(); this->next()) {
            for (auto expr_it = exprs.begin();
                 expr_it != exprs.end(); ++expr_it) {
                COLOG_DEBUG(*expr_it);
                result = boost::apply_visitor(eval_visitor(inner), *expr_it);
            }
        }
        return result;
    }
};

Object eval_loop_step(varlisp::Environment& env,
                      const varlisp::symbol& sym,
                      const varlisp::Object& start,
                      const varlisp::Object& end,
                      const varlisp::Object& step,
                      const varlisp::List exprs)
{
    // varlisp::List condition;
    // varlisp::List next;
    Object zero{int64_t(0)};
    bool use_less = boost::apply_visitor(strict_less_visitor(env), zero, step);

    Object conditionObj(
        varlisp::List({symbol(use_less ? "<" : ">"), sym, end}));
    Object nextObj(varlisp::List(
        {symbol("setq"), sym,
         varlisp::List({symbol("+"), sym, step})}));
    loop_ctrl_t loop_ctrl(env, sym.name(), start, conditionObj, nextObj);

    return loop_ctrl.loop(env, exprs);
}


Object eval_loop_condition(varlisp::Environment& env,
                           const varlisp::List* p_ctrl_block,
                           const varlisp::List exprs)
{
    // ((v1 a1 v2 ...)
    //  condition
    //  expr // NOTE 如果expr这里要执行多条语句，必须使用(begin 语句)
    // )
    if (p_ctrl_block->length() > 3) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(car: noly 3 element need; but",
                           *p_ctrl_block, ")");
    }

    COLOG_DEBUG(detail::car(*p_ctrl_block));
    COLOG_DEBUG(detail::cadr(*p_ctrl_block));
    COLOG_DEBUG(detail::caddr(*p_ctrl_block));
    COLOG_DEBUG(exprs);

    loop_ctrl_t loop_ctrl(env, p_ctrl_block);

    return loop_ctrl.loop(env, exprs);
}

REGIST_BUILTIN("begin", 1, -1, eval_begin,
               "; begin 批量执行\n"
               "(begin expr...) -> result-of-last-expr");

Object eval_begin(varlisp::Environment& env, const varlisp::List& args)
{
    Object result = Nill{};

    for (auto expr_it = args.begin();
         expr_it != args.end(); ++expr_it)
    {
        COLOG_DEBUG(*expr_it);
        result = boost::apply_visitor(eval_visitor(env), *expr_it);
    }
    
    return result;
}

REGIST_BUILTIN("silent", 1, -1, eval_silent,
               "; silent silent批量执行\n"
               "(silent expr...) -> result-of-last-expr");

Object eval_silent(varlisp::Environment& env, const varlisp::List& args)
{
    try {
        // NOTE 是否需要每个语句，都catch一下？
        // 如果语句之间，有逻辑上的先后关系呢？
        return eval_begin(env, args);
    }
    catch (std::runtime_error& e) {
        COLOG_ERROR(e.what());
    }
    catch( std::exception& e) {
        COLOG_ERROR(e.what());
    }
    catch(...) {
        COLOG_ERROR("unkown exception");
    }
    return Nill{};
}

// REGIST_BUILTIN("while", 1, -1, eval_while,
//                "; while 循环执行\n"
//                "(for condition expr...) -> result-of-last-expr");
// 
// Object eval_while(varlisp::Environment& env, const varlisp::List& args)
// {
//     
// }

REGIST_BUILTIN("tie", 2, 2, eval_tie,
               "; tie 拆分\n"
               "; 类似python; 如果两遍，符号和list长度不匹配：\n"
               "; 名字多，则多余的取nil; 如果list多，则多余的取tail，赋值给最后一个元素\n"
               "(tie ('var-list...) [list...])");

Object eval_tie(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "tie";
    std::array<Object, 2> objs;
    auto * p_var_list = boost::get<varlisp::List>(&args.nth(0));
    varlisp::requireOnFaild<varlisp::List>(p_var_list, funcName, 0, DEBUG_INFO);
    auto * p_value_list = varlisp::getQuotedList(env, args.nth(1), objs[1]);
    varlisp::requireOnFaild<varlisp::QuoteList>(p_value_list, funcName, 1, DEBUG_INFO);

    varlisp::List tmp_list;
    tmp_list.append_list(env, *p_value_list);

    const varlisp::symbol * p_sym = nullptr;
    for (size_t i = 0; i < p_var_list->size(); ++i) {
        p_sym = boost::get<varlisp::symbol>(&p_var_list->nth(i));
        varlisp::requireOnFaild<varlisp::symbol>(p_sym, funcName, i, DEBUG_INFO);

        if (i >= tmp_list.size()) {
            env[p_sym->name()] = varlisp::Nill{};
            continue;
        }

        if (i != p_var_list->size() - 1 || p_var_list->size() == tmp_list.size()) {
            env[p_sym->name()] = tmp_list.nth(i);
        }
        else {
            if (p_var_list->size() < tmp_list.size()) {
                if (i == 0) {
                    env[p_sym->name()] = varlisp::List::makeSQuoteObj(std::move(tmp_list));
                }
                else {
                    env[p_sym->name()] = varlisp::List::makeSQuoteObj(std::move(tmp_list.tail(i - 1)));
                }
            }
        }
    }

    if (p_sym) {
        return env[p_sym->name()];
    }
    else {
        return varlisp::List::makeSQuoteObj(std::move(tmp_list));
    }
}

} // namespace varlisp
