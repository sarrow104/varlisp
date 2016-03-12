#include "eval_visitor.hpp"

#include "list.hpp"

#include <sss/utlstring.hpp>

namespace varlisp {
    Object eval_visitor::operator() (const Define& d) const
    {
        m_env[d.name.m_data] = boost::apply_visitor(eval_visitor(m_env), d.value);
        return Object();
    }

    Object eval_visitor::operator() (const IfExpr& i) const
    {
        return i.eval(m_env);
    }

    Object eval_visitor::operator() (const List& l) const
    {
        return l.eval(m_env);
    }
    
} // namespace varlisp
