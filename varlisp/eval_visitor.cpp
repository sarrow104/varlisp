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
        Object condition = boost::apply_visitor(eval_visitor(m_env), i.condition);
        if (int * p_b = boost::get<int>(&condition)) {
            condition = bool(*p_b);
        }
        else if (double * p_d = boost::get<double>(&condition)) {
            condition = bool(*p_d);
        }
        else if (std::string * p_s = boost::get<std::string>(&condition)) {
            condition = bool(sss::string_cast_nothrow<int>(*p_s));
        }
        else if (List * p_l = boost::get<List>(&condition)) {
            condition = bool(!p_l->is_empty());
        }
        else {
            condition = false;
        }

        if (condition.which() != 1) {
            throw std::runtime_error("must bool eval!");
        }

        if (boost::get<bool>(condition)) {
            return boost::apply_visitor(eval_visitor(m_env), i.consequent);
        }
        else {
            return boost::apply_visitor(eval_visitor(m_env), i.alternative);
        }
    }

    Object eval_visitor::operator() (const List& l) const
    {
        return l.eval(m_env);
    }
    
} // namespace varlisp
