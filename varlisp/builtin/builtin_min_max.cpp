#include "object.hpp"

#include "strict_less_visitor.hpp"
#include "builtin_helper.hpp"
#include "detail/buitin_info_t.hpp"
#include "detail/list_iterator.hpp"
#include "detail/car.hpp"

// NOTE 为什么不把min,max放mapreduce下面呢？
// map,reduce系列的特点：
// 1. 本质上是超越函数。
// 2. 需要传入操作的函数。
// 3. 参数之间没有直接的联系；
//
// 而max,min函数
// 1. 无须传入函数；
// 2. 不同参数之间，有比较的关系；
//
// 因此，从分类上来说，更接近sort；

namespace varlisp {

namespace detail {

template<typename FuncT>
Object min_max_impl(varlisp::Environment& env, const varlisp::List& args,
                    const char * funcName, const FuncT& func)
{
    Object objLeft;
    Object objRight;
    const varlisp::List * p_list = nullptr;
    if (args.length() == 1) {
        p_list = varlisp::getQuotedList(env, detail::car(args), objLeft);
        if (!p_list || p_list->empty()) {
            SSS_POSITION_THROW(
                std::runtime_error, "(", funcName,
                ": the only one argument, must be none-empty s-list!; but ",
                *p_list, ")");
        }
    }
    else {
        p_list = &args;
    }

    auto it = p_list->begin();

    Object minRes = varlisp::getAtomicValue(env, *it, objLeft);
    for ( ++it; it != p_list->end(); ++it)
    {
        const Object& rightRef = varlisp::getAtomicValue(env, *it, objRight);
        if (func(env, rightRef, minRes)) {
            minRes = rightRef;
        }
    }
    return minRes;
}

} // namespace detail

REGIST_BUILTIN("min", 1, -1, eval_min,
               "(min [obj1 obj2...]) -> the-minimum-element\n"
               "(min obj1 obj2...) -> the-minimum-element");

Object eval_min(varlisp::Environment& env, const varlisp::List& args)
{
    return detail::min_max_impl(env, args, "min",
                                [](Environment& env, const Object& lhs, const Object& rhs)->bool {
                                    return boost::apply_visitor(strict_less_visitor(env), lhs, rhs);
                                });
}

REGIST_BUILTIN("max", 1, -1, eval_max,
               "(max [obj1 obj2...]) -> the-maximum-element\n"
               "(max obj1 obj2...) -> the-maximum-element");

Object eval_max(varlisp::Environment& env, const varlisp::List& args)
{
    return detail::min_max_impl(env, args, "min",
                                [](Environment& env, const Object& lhs, const Object& rhs)->bool {
                                    return boost::apply_visitor(strict_less_visitor(env), rhs, lhs);
                                });
}


} // namespace verlisp
