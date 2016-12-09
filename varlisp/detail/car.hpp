#pragma once

#include "../object.hpp"

namespace varlisp {
namespace detail {
inline const Object& car(const varlisp::List& args)
{
    return args.head();
}

inline Object& car(varlisp::List& args)
{
    return args.head();
}

inline const Object& cadr(const varlisp::List& args)
{
    return args.tail().head();
}

inline Object& cadr(varlisp::List& args)
{
    return args.tail().head();
}

inline const Object& caddr(const varlisp::List& args)
{
    return args.tail().tail().head();
}

inline Object& caddr(varlisp::List& args)
{
    return args.tail().tail().head();
}

inline varlisp::List tail(const varlisp::List & args)
{
    return args.tail();
}

inline varlisp::List tail(varlisp::List & args)
{
    return args.tail();
}

} // namespace detail

} // namespace varlisp
