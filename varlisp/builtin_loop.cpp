#include <stdexcept>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>

#include "object.hpp"

#include "builtin_helper.hpp"
#include "strict_less_visitor.hpp"
#include "cast2bool_visitor.hpp"
#include "detail/buitin_info_t.hpp"
#include "detail/car.hpp"
#include "detail/list_iterator.hpp"


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
                      const varlisp::List* p_slist,
                      const varlisp::List* p_exprs);

Object eval_loop_step(varlisp::Environment& env,
                      const varlisp::symbol& sym,
                      const varlisp::Object& start,
                      const varlisp::Object& end,
                      const varlisp::Object& step,
                      const varlisp::List* p_exprs);

Object eval_loop_condition(varlisp::Environment& env,
                           const varlisp::List* p_ctrl_block,
                           const varlisp::List* p_exprs);

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
    if (p_loop_ctrl->is_squote()) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must not be a s-list; but",
                           detail::car(args), ")");
    }
    COLOG_DEBUG(SSS_VALUE_MSG(p_loop_ctrl->is_squote()));
    COLOG_DEBUG(SSS_VALUE_MSG(detail::car(*p_loop_ctrl)));
    int ctrl_block_len = p_loop_ctrl->length();
    COLOG_DEBUG(SSS_VALUE_MSG(ctrl_block_len));
    Object tmpSym;
    // const varlisp::symbol * p_sym = varlisp::getSymbol(env, detail::car(*p_loop_ctrl), tmpSym);
    const varlisp::symbol* p_sym =
        boost::get<varlisp::symbol>(&detail::car(*p_loop_ctrl));

    COLOG_DEBUG(SSS_VALUE_MSG(ctrl_block_len));
    if (2 == ctrl_block_len) {
        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": a symbol is_needed as iterator arg; but",
                               detail::car(*p_loop_ctrl), ")");
        }
        Object tmpSList;
        const varlisp::List* p_slist = varlisp::getFirstListPtrFromArg(
            env, *p_loop_ctrl->next(), tmpSList);
        if (!p_slist || !p_slist->is_squote()) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": a slist is_needed as iterator range; but",
                               detail::cadr(*p_loop_ctrl), ")");
        }
        return eval_loop_list(env, *p_sym, p_slist->next(), args.next());
    }
    else if (4 == ctrl_block_len || (3 == ctrl_block_len && p_sym)) {
        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": a symbol is_needed as iterator arg; but",
                               detail::car(*p_loop_ctrl), ")");
        }
        p_loop_ctrl = p_loop_ctrl->next();
        Object tmpStart;
        Object tmpEnd;
        Object tmpStep{int64_t(1)};
        return eval_loop_step(
            env, *p_sym,
            varlisp::getAtomicValue(env, detail::car(*p_loop_ctrl), tmpStart),
            varlisp::getAtomicValue(env, detail::cadr(*p_loop_ctrl), tmpEnd),
            ctrl_block_len == 4 ? varlisp::getAtomicValue(
                                      env, detail::caddr(*p_loop_ctrl), tmpStep)
                                : tmpStep,
            args.next());
    }
    else if (3 == ctrl_block_len && !p_sym) {
        return eval_loop_condition(env, p_loop_ctrl, args.next());
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": invalid arguments: ", args, ")");
    }
}

Object eval_loop_list(varlisp::Environment& env,
                      const varlisp::symbol& sym,
                      const varlisp::List* p_slist,
                      const varlisp::List* p_exprs)
{
    varlisp::Environment inner(&env);
    Object result;
    for (auto it = detail::list_object_const_iterator_t(p_slist); it; ++it) {
        inner[sym.m_data] = *it;
        // FIXME keywords check!
        for (const varlisp::List* p_expr = p_exprs;
             p_expr && p_exprs->head.which();
             p_expr = p_expr->next())
        {
            result = boost::apply_visitor(eval_visitor(inner), p_expr->head);
        }
    }
    return result;
}

struct loop_ctrl_t
{
private:
    varlisp::Environment& m_env;
    std::vector<std::string> m_sym_vec;
    std::vector<varlisp::Object> m_init_value_vec;
    const varlisp::Object & m_condition;
    const varlisp::Object & m_next;

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
            boost::get<varlisp::List>(&p_ctrl_block->head);
        if (!p_kv_pair_list) {
            SSS_POSITION_THROW(std::runtime_error, p_ctrl_block->head);
        }
        const varlisp::List * p_var = p_kv_pair_list;
        const varlisp::List * p_val = p_var->next();
        while(detail::is_car_valid(p_var)) {
            Object tmpObj;
            const varlisp::symbol * p_sym = varlisp::getSymbol(
                m_env,
                p_var->head,
                tmpObj);
            if (!p_sym) {
                SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                                   ": 1st must be a symbol; but ", p_var->head,
                                   ")");
            }
            if (!detail::is_car_valid(p_val)) {
                SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                                   ": 2nd value for ", *p_sym, " is empty! '())");
            }
            m_sym_vec.push_back(p_sym->m_data);
            Object res;
            m_init_value_vec.push_back(getAtomicValue(env, p_val->head, res));
            p_var = p_val->next();
            if (p_var) {
                p_val = p_var->next();
            }
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
            COLOG_DEBUG(m_sym_vec[i], '=', m_init_value_vec[i]);
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
};

Object eval_loop_step(varlisp::Environment& env,
                      const varlisp::symbol& sym,
                      const varlisp::Object& start,
                      const varlisp::Object& end,
                      const varlisp::Object& step,
                      const varlisp::List* p_exprs)
{
    // varlisp::List condition;
    // varlisp::List next;
    Object zero{int64_t(0)};
    bool use_less = boost::apply_visitor(strict_less_visitor(env), zero, step);

    varlisp::Environment inner(&env);
    Object conditionObj(
        varlisp::List::makeList({symbol(use_less ? "<" : ">"), sym, end}));
    Object nextObj(varlisp::List::makeList(
        {symbol("setq"), sym,
         varlisp::List::makeList({symbol("+"), sym, step})}));
    loop_ctrl_t loop_ctrl(env, sym.m_data, start, conditionObj, nextObj);
    Object result = Nill{};

    for (loop_ctrl.start(); loop_ctrl.condition(); loop_ctrl.next()) {
        for (const varlisp::List* p_expr = p_exprs;
             detail::is_car_valid(p_expr); p_expr = p_expr->next()) {
            COLOG_DEBUG(p_expr->head);
            result = boost::apply_visitor(eval_visitor(inner), p_expr->head);
        }
    }
    return result;
}


Object eval_loop_condition(varlisp::Environment& env,
                           const varlisp::List* p_ctrl_block,
                           const varlisp::List* p_exprs)
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

    varlisp::Environment inner(&env);
    loop_ctrl_t loop_ctrl(env, p_ctrl_block);
    Object result = Nill{};

    for (loop_ctrl.start(); loop_ctrl.condition(); loop_ctrl.next()) {
        for (const varlisp::List* p_expr = p_exprs;
             detail::is_car_valid(p_expr);
             p_expr = p_expr->next())
        {
            COLOG_DEBUG(p_expr->head);
            result = boost::apply_visitor(eval_visitor(inner), p_expr->head);
        }
    }
    
    return result;
}

REGIST_BUILTIN("begin", 1, -1, eval_begin,
               "; begin 批量执行\n"
               "(begin expr...) -> result-of-last-expr");

Object eval_begin(varlisp::Environment& env, const varlisp::List& args)
{
    Object result = Nill{};

    for (const varlisp::List* p_expr = &args;
         detail::is_car_valid(p_expr);
         p_expr = p_expr->next())
    {
        COLOG_DEBUG(p_expr->head);
        result = boost::apply_visitor(eval_visitor(env), p_expr->head);
    }
    
    return result;
}

} // namespace varlisp
