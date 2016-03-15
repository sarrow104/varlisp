#ifndef __EVAL_VISITOR_HPP_1457603665__
#define __EVAL_VISITOR_HPP_1457603665__

#include "object.hpp"

#include <sss/util/PostionThrow.hpp>

namespace varlisp {
    struct Environment;
struct eval_visitor : boost::static_visitor<Object>
{
    Environment & m_env;
    explicit eval_visitor(Environment& env)
        : m_env(env)
    {
    }

    template <typename T>
    Object operator() (const T& v) const
    {
        return v.eval(m_env);
    }

    Object operator() (const varlisp::symbol& s) const;

    Object operator() (const Empty& ) const
    {
        return Object();
    }

    Object operator() (bool v) const
    {
        return v;
    }

    Object operator() (int v) const
    {
        return v;
    }

    Object operator() (double v) const
    {
        return v;
    }

    Object operator() (const std::string v) const
    {
        return v;
    }

    Object operator() (const varlisp::Builtin& v) const
    {
        return v;
    }

    Object operator() (const varlisp::Lambda& v) const
    {
        return v;
    }
};
} // namespace varlisp


#endif /* __EVAL_VISITOR_HPP_1457603665__ */
