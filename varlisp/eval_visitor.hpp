#ifndef __EVAL_VISITOR_HPP_1457603665__
#define __EVAL_VISITOR_HPP_1457603665__

#include "object.hpp"

#include "environment.hpp"

#include <sss/util/PostionThrow.hpp>

namespace varlisp {
struct eval_visitor : boost::static_visitor<Object>
{
    Environment & m_env;
    eval_visitor(Environment& env)
        : m_env(env)
    {
    }

    template <typename T>
    Object operator() (const T& v) const
    {
        return v;
    }

    Object operator() (const Empty& ) const
    {
        return Object();
    }

    Object operator() (const varlisp::symbol& s) const
    {
        Environment::const_iterator it = m_env.find(s.m_data);
        if (it == m_env.cend()) {
            SSS_POSTION_THROW(std::runtime_error,
                              "symbol " << s.m_data << " not exsist");
        }
        return it->second;
    }

    Object operator() (const Define& d) const;

    Object operator() (const IfExpr& i) const;

    Object operator() (const List& l) const;

    // Object operator() (const Lambda& l) const;
};
} // namespace varlisp


#endif /* __EVAL_VISITOR_HPP_1457603665__ */
