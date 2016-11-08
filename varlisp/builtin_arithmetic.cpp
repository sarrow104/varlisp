#include "object.hpp"

#include "cast2double_visitor.hpp"

namespace varlisp {

Object eval_add(varlisp::Environment& env, const varlisp::List& args)
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    double sum = 0;
    const List* p = &args;
    while (p && p->head.which()) {
        sum += boost::apply_visitor(cast2double_visitor(env), p->head);
        if (p->tail.empty()) {
            p = 0;
        }
        else {
            p = &p->tail[0];
        }
    }
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, sum);
    return Object(sum);
}

Object eval_sub(varlisp::Environment& env, const varlisp::List& args)
{
    int args_cnt = args.length();
    if (args_cnt == 1) {
        return Object(
            -boost::apply_visitor(cast2double_visitor(env), args.head));
    }
    else {
        double sum = boost::apply_visitor(cast2double_visitor(env), args.head);
        const List* p = &args.tail[0];
        while (p && p->head.which()) {
            sum -= boost::apply_visitor(cast2double_visitor(env), p->head);
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
        return Object(sum);
    }
}

Object eval_mul(varlisp::Environment& env, const varlisp::List& args)
{
    double mul = boost::apply_visitor(cast2double_visitor(env), args.head);
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, mul);
    const List* p = &args.tail[0];
    while (mul && p && p->head.which()) {
        mul *= boost::apply_visitor(cast2double_visitor(env), p->head);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, mul);
        if (p->tail.empty()) {
            p = 0;
        }
        else {
            p = &p->tail[0];
        }
    }
    return Object(mul);
}

Object eval_div(varlisp::Environment& env, const varlisp::List& args)
{
    double mul = boost::apply_visitor(cast2double_visitor(env), args.head);
    const List* p = &args.tail[0];
    while (mul && p && p->head.which()) {
        double div = boost::apply_visitor(cast2double_visitor(env), p->head);
        if (!div) {
            throw std::runtime_error(" divide by zero!");
        }
        mul /= div;
        if (p->tail.empty()) {
            p = 0;
        }
        else {
            p = &p->tail[0];
        }
    }
    return Object(mul);
}

Object eval_pow(varlisp::Environment& env, const varlisp::List& args)
{
    double lhs = boost::apply_visitor(cast2double_visitor(env), args.head);
    double rhs =
        boost::apply_visitor(cast2double_visitor(env), args.tail[0].head);

    return Object(std::pow(lhs, rhs));
}

}  // namespace varlisp
