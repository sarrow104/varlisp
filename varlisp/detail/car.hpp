#pragma once

#include "../object.hpp"

namespace varlisp {
namespace detail {
inline const Object& car(const varlisp::List& args)
{
    return args.head;
}

inline Object& car(varlisp::List& args)
{
    return args.head;
}

inline const Object& cadr(const varlisp::List& args)
{
    return args.tail[0].head;
}

inline Object& cadr(varlisp::List& args)
{
    return args.tail[0].head;
}

inline const Object& caddr(const varlisp::List& args)
{
    return args.tail[0].tail[0].head;
}

inline Object& caddr(varlisp::List& args)
{
    return args.tail[0].tail[0].head;
}

inline bool is_car_valid(const varlisp::List * p_list)
{
    return p_list && p_list->head.which();
}

inline bool is_car_valid(const varlisp::List & list)
{
    return list.head.which();
}

inline const varlisp::List * tail(const varlisp::List & args)
{
    if (is_car_valid(args) && !args.tail.empty()) {
        return &args.tail[0];
    }
    else {
        return nullptr;
    }
}

inline varlisp::List * tail(varlisp::List & args)
{
    if (is_car_valid(args) && !args.tail.empty()) {
        return &args.tail[0];
    }
    else {
        return nullptr;
    }
}

} // namespace detail

} // namespace varlisp
