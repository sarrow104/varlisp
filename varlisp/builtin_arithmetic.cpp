#include "arithmetic_cast_visitor.hpp"
#include "is_instant_visitor.hpp"
#include "object.hpp"

#include <sss/debug/value_msg.hpp>
#include <boost/variant/multivisitors.hpp>
#include <limits>

namespace varlisp {

template <typename T1, typename T2, typename Tr>
struct binary_add {
    auto operator()(T1 v1, T2 v2) -> decltype(v1 + v2) const
    {
        return Tr(v1 + v2);
    }
};

// DrRachet 中，只有int和double 可以+，-运算。
// number? 运算符
// 字符串与字符串，都是不行的！
//
struct arithmetic_add_visitor : public boost::static_visitor<arithmetic_t> {
    template <typename T2>
    arithmetic_t operator()(const Empty&, const T2&) const
    {
        return arithmetic_t{};
    }

    template <typename T1>
    arithmetic_t operator()(const T1&, const Empty&) const
    {
        return arithmetic_t{};
    }

    arithmetic_t operator()(const Empty&, const Empty&) const
    {
        return arithmetic_t{};
    }

    template <typename T1, typename T2>
    arithmetic_t operator()(const T1& v1, const T2& v2) const
    {
        return v1 + v2;
    }
};

struct arithmetic_sub_visitor : public boost::static_visitor<arithmetic_t> {
    template <typename T2>
    arithmetic_t operator()(const Empty&, const T2&) const
    {
        return arithmetic_t{};
    }

    template <typename T1>
    arithmetic_t operator()(const T1&, const Empty&) const
    {
        return arithmetic_t{};
    }

    arithmetic_t operator()(const Empty&, const Empty&) const
    {
        return arithmetic_t{};
    }

    template <typename T1, typename T2>
    arithmetic_t operator()(const T1& v1, const T2& v2) const
    {
        return v1 - v2;
    }
};
struct arithmetic_mul_visitor : public boost::static_visitor<arithmetic_t> {
    template <typename T2>
    arithmetic_t operator()(const Empty&, const T2&) const
    {
        return arithmetic_t{};
    }

    template <typename T1>
    arithmetic_t operator()(const T1&, const Empty&) const
    {
        return arithmetic_t{};
    }

    arithmetic_t operator()(const Empty&, const Empty&) const
    {
        return arithmetic_t{};
    }

    template <typename T1, typename T2>
    arithmetic_t operator()(const T1& v1, const T2& v2) const
    {
        return v1 * v2;
    }
};

// NOTE 这里的除法运算规律，同C/C++。
// 就是说 2/5 == 0
struct arithmetic_div_visitor : public boost::static_visitor<arithmetic_t> {
    template <typename T2>
    arithmetic_t operator()(const Empty&, const T2&) const
    {
        return arithmetic_t{};
    }

    template <typename T1>
    arithmetic_t operator()(const T1&, const Empty&) const
    {
        return arithmetic_t{};
    }

    arithmetic_t operator()(const Empty&, const Empty&) const
    {
        return arithmetic_t{};
    }

    template <typename T1, typename T2>
    arithmetic_t operator()(const T1& v1, const T2& v2) const
    {
        if (v2 == 0) {
            SSS_POSTION_THROW(std::runtime_error, v1, "divided by Zero");
        }
        return v1 / v2;
    }
};

// DrRachet + 支持0个参数；-需要至少一个参数；
Object eval_add(varlisp::Environment& env, const varlisp::List& args)
{
    arithmetic_t sum = 0;
    const List* p = &args;
    while (sum.which() && p && p->head.which()) {
        arithmetic_t to_add =
            boost::apply_visitor(arithmetic_cast_visitor(env), p->head);
        sum = boost::apply_visitor(arithmetic_add_visitor(), sum, to_add);
        p = p->next();
    }

    return arithmetic2object(sum);
}

Object eval_sub(varlisp::Environment& env, const varlisp::List& args)
{
    int args_cnt = args.length();
    if (args_cnt == 1) {
        arithmetic_t sum{0};
        arithmetic_t to_sub =
            boost::apply_visitor(arithmetic_cast_visitor(env), args.head);
        return arithmetic2object(
            boost::apply_visitor(arithmetic_sub_visitor(), sum, to_sub));
    }
    else {
        arithmetic_t sum =
            boost::apply_visitor(arithmetic_cast_visitor(env), args.head);
        const List* p = &args.tail[0];
        while (sum.which() && p && p->head.which()) {
            arithmetic_t to_sub =
                boost::apply_visitor(arithmetic_cast_visitor(env), p->head);
            sum = boost::apply_visitor(arithmetic_sub_visitor(), sum, to_sub);
            p = p->next();
        }
        return Object(sum);
    }
}

Object eval_mul(varlisp::Environment& env, const varlisp::List& args)
{
    arithmetic_t mul{1};
    const List* p = &args;
    while (mul.which() && p && p->head.which()) {
        arithmetic_t to_mul =
            boost::apply_visitor(arithmetic_cast_visitor(env), p->head);
        mul = boost::apply_visitor(arithmetic_mul_visitor(), mul, to_mul);
        p = p->next();
    }

    return arithmetic2object(mul);
}

Object eval_div(varlisp::Environment& env, const varlisp::List& args)
{
    if (args.length() == 1) {
        arithmetic_t mul{1};
        arithmetic_t to_div =
            boost::apply_visitor(arithmetic_cast_visitor(env), args.head);
        return arithmetic2object(boost::apply_visitor(arithmetic_div_visitor(), mul, to_div));
    }
    else {
        arithmetic_t mul = boost::apply_visitor(arithmetic_cast_visitor(env), args.head);
        const List* p = &args.tail[0];
        while (mul.which() && p && p->head.which()) {
            arithmetic_t to_div = boost::apply_visitor(arithmetic_cast_visitor(env), p->head);
            mul = boost::apply_visitor(arithmetic_div_visitor(), mul, to_div);
            p = p->next();
        }
        return arithmetic2object(mul);
    }
}

Object eval_pow(varlisp::Environment& env, const varlisp::List& args)
{
    double lhs = arithmetic2double(boost::apply_visitor(arithmetic_cast_visitor(env), args.head));
    double rhs = arithmetic2double(boost::apply_visitor(arithmetic_cast_visitor(env), args.tail[0].head));

    return Object(std::pow(lhs, rhs));
}

}  // namespace varlisp
