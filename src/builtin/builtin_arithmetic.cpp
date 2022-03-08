#include <limits>

#include <boost/variant/multivisitors.hpp>

#include <sss/debug/value_msg.hpp>

#include "../arithmetic_cast_visitor.hpp"
#include "../is_instant_visitor.hpp"
#include "../object.hpp"
#include "../detail/car.hpp"

#include "../detail/buitin_info_t.hpp"

namespace varlisp {

template <typename T1, typename T2, typename Tr>
struct binary_add {
    auto operator()(T1 v1, T2 v2) -> decltype(v1 + v2)
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
    arithmetic_t operator()(const Empty& /*unused*/, const T2& /*unused*/) const
    {
        return arithmetic_t{};
    }

    template <typename T1>
    arithmetic_t operator()(const T1& /*unused*/, const Empty& /*unused*/) const
    {
        return arithmetic_t{};
    }

    arithmetic_t operator()(const Empty& /*unused*/, const Empty& /*unused*/) const
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
    arithmetic_t operator()(const Empty& /*unused*/, const T2& /*unused*/) const
    {
        return arithmetic_t{};
    }

    template <typename T1>
    arithmetic_t operator()(const T1& /*unused*/, const Empty& /*unused*/) const
    {
        return arithmetic_t{};
    }

    arithmetic_t operator()(const Empty& /*unused*/, const Empty& /*unused*/) const
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
    arithmetic_t operator()(const Empty& /*unused*/, const T2& /*unused*/) const
    {
        return arithmetic_t{};
    }

    template <typename T1>
    arithmetic_t operator()(const T1& /*unused*/, const Empty& /*unused*/) const
    {
        return arithmetic_t{};
    }

    arithmetic_t operator()(const Empty& /*unused*/, const Empty& /*unused*/) const
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
    arithmetic_t operator()(const Empty& /*unused*/, const T2& /*unused*/) const
    {
        return arithmetic_t{};
    }

    template <typename T1>
    arithmetic_t operator()(const T1& /*unused*/, const Empty& /*unused*/) const
    {
        return arithmetic_t{};
    }

    arithmetic_t operator()(const Empty& /*unused*/, const Empty& /*unused*/) const
    {
        return arithmetic_t{};
    }

    template <typename T1, typename T2>
    arithmetic_t operator()(const T1& v1, const T2& v2) const
    {
        if (v2 == 0) {
            SSS_POSITION_THROW(std::runtime_error, v1, "divided by Zero");
        }
        return v1 / v2;
    }
};

// NOTE 这里的求余数运算规律，同C/C++。
// 就是说 1/5 == 0
struct arithmetic_mod_visitor : public boost::static_visitor<arithmetic_t> {
    template <typename T2>
    arithmetic_t operator()(const Empty& /*unused*/, const T2& /*unused*/) const
    {
        return arithmetic_t{};
    }

    template <typename T1>
    arithmetic_t operator()(const T1& /*unused*/, const Empty& /*unused*/) const
    {
        return arithmetic_t{};
    }

    arithmetic_t operator()(const Empty& /*unused*/, const Empty& /*unused*/) const
    {
        return arithmetic_t{};
    }

    arithmetic_t operator()(int64_t v1, int64_t v2) const
    {
        if (v2 == 0) {
            SSS_POSITION_THROW(std::runtime_error, v1, "remaind by Zero");
        }
        return v1 % v2;
    }
    arithmetic_t operator()(double v1, int64_t v2) const
    {
        if (v2 == 0) {
            SSS_POSITION_THROW(std::runtime_error, v1, "remaind by Zero");
        }
        return int64_t(v1) % v2;
    }
    arithmetic_t operator()(int64_t v1, double v2) const
    {
        if (v2 == 0) {
            SSS_POSITION_THROW(std::runtime_error, v1, "remaind by Zero");
        }
        return std::fmod(double(v1), v2);
    }
    arithmetic_t operator()(double v1, double v2) const
    {
        if (v2 == 0) {
            SSS_POSITION_THROW(std::runtime_error, v1, "remaind by Zero");
        }
        return std::fmod(v1, v2);
    }
};

REGIST_BUILTIN("+", 0, -1, eval_add, "(+ ...) -> number");

// DrRachet + 支持0个参数；-需要至少一个参数；
Object eval_add(varlisp::Environment& env, const varlisp::List& args)
{
    arithmetic_t sum = int64_t(0);

    for (auto it = args.begin(); (sum.which() != 0) && it != args.end(); ++it) {
        arithmetic_t to_add =
            boost::apply_visitor(arithmetic_cast_visitor(env), *it);
        sum = boost::apply_visitor(arithmetic_add_visitor(), sum, to_add);
    }

    return arithmetic2object(sum);
}

REGIST_BUILTIN("-", 0, -1, eval_sub, "(- arg ...) -> number");

Object eval_sub(varlisp::Environment& env, const varlisp::List& args)
{
    auto args_cnt = args.length();
    if (args_cnt == 1) {
        arithmetic_t sum{int64_t(0)};
        arithmetic_t to_sub =
            boost::apply_visitor(arithmetic_cast_visitor(env), detail::car(args));
        return arithmetic2object(
            boost::apply_visitor(arithmetic_sub_visitor(), sum, to_sub));
    }
    arithmetic_t sum =
        boost::apply_visitor(arithmetic_cast_visitor(env), detail::car(args));

    const List tail = args.tail();

    for (auto it = tail.begin(); (sum.which() != 0) && it != tail.end(); ++it) {
        arithmetic_t to_sub =
            boost::apply_visitor(arithmetic_cast_visitor(env), *it);
        sum = boost::apply_visitor(arithmetic_sub_visitor(), sum, to_sub);
    }
    return {sum};
}

REGIST_BUILTIN("*", 0, -1, eval_mul, "(* ...) -> number");

Object eval_mul(varlisp::Environment& env, const varlisp::List& args)
{
    arithmetic_t mul{int64_t(1)};

    for (auto it = args.begin(); (mul.which() != 0) && it != args.end(); ++it) {
        arithmetic_t to_mul =
            boost::apply_visitor(arithmetic_cast_visitor(env), *it);
        mul = boost::apply_visitor(arithmetic_mul_visitor(), mul, to_mul);
    }

    return arithmetic2object(mul);
}

REGIST_BUILTIN("/", 1, -1, eval_div, "(/ arg ...) -> number");

Object eval_div(varlisp::Environment& env, const varlisp::List& args)
{
    if (args.length() == 1) {
        arithmetic_t mul{int64_t(1)};
        arithmetic_t to_div =
            boost::apply_visitor(arithmetic_cast_visitor(env), detail::car(args));
        return arithmetic2object(boost::apply_visitor(arithmetic_div_visitor(), mul, to_div));
    }
    arithmetic_t mul = boost::apply_visitor(arithmetic_cast_visitor(env), detail::car(args));

    const List tail = args.tail();
    for (auto it = tail.begin(); (mul.which() != 0) && it != tail.end(); ++it) {
        arithmetic_t to_div = boost::apply_visitor(arithmetic_cast_visitor(env), *it);
        mul = boost::apply_visitor(arithmetic_div_visitor(), mul, to_div);
    }

    return arithmetic2object(mul);
}

REGIST_BUILTIN("%", 2, 2, eval_mod, "(%" " arg1 arg2) -> number");

Object eval_mod(varlisp::Environment& env, const varlisp::List& args)
{
    arithmetic_t lhs = boost::apply_visitor(arithmetic_cast_visitor(env), args.nth(0));
    arithmetic_t rhs = boost::apply_visitor(arithmetic_cast_visitor(env), args.nth(1));

    return arithmetic2object(boost::apply_visitor(arithmetic_mod_visitor(), lhs, rhs));
}

REGIST_BUILTIN("power", 2, 2, eval_pow, "(power arg1 arg2) -> number");

Object eval_pow(varlisp::Environment& env, const varlisp::List& args)
{
    double lhs = arithmetic2double(
        boost::apply_visitor(arithmetic_cast_visitor(env), detail::car(args)));
    double rhs = arithmetic2double(
        boost::apply_visitor(arithmetic_cast_visitor(env), detail::cadr(args)));

    return {std::pow(lhs, rhs)};
}

}  // namespace varlisp
