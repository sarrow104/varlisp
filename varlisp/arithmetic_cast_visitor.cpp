#include "arithmetic_cast_visitor.hpp"

#include <cstdlib>

#include <sss/raw_print.hpp>

#include "is_instant_visitor.hpp"
#include "eval_visitor.hpp"
#include "object.hpp"

namespace varlisp {

varlisp::arithmetic_t arithmetic_cast_visitor::operator()(
    const std::string& s) const
{
    int64_t iv = 0;
    double dv = 0;
    int offset = -1;
    if (std::sscanf(s.c_str(), "%ld%n", &iv, &offset) == 1 &&
        offset == int64_t(s.length())) {
        return iv;
    }
    else if (std::sscanf(s.c_str(), "%lf%n", &dv, &offset) == 1 &&
             offset == int64_t(s.length())) {
        return dv;
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, "failed convert ",
                          sss::raw_string(s), " to arithmetic_t");
    }
}

varlisp::arithmetic_t arithmetic_cast_visitor::operator()(const varlisp::symbol& s) const
{
    Object* p_obj = m_env.find(s.m_data);
    if (!p_obj) {
        SSS_POSITION_THROW(std::runtime_error, "symbol ", s.m_data,
                          " not exists!");
    }
    Object tmp;
    if (boost::apply_visitor(is_instant_visitor(m_env), *p_obj)) {
        return boost::apply_visitor(arithmetic_cast_visitor(m_env), *p_obj);
    }
    else {
        tmp = boost::apply_visitor(eval_visitor(m_env), *p_obj);
        return boost::apply_visitor(arithmetic_cast_visitor(m_env), tmp);
    }
}

varlisp::arithmetic_t arithmetic_cast_visitor::operator()(const List& l) const
{
    if (l.is_squote()) {
        SSS_POSITION_THROW(std::runtime_error,
                          "object: cannot convert from s-list to arithmetic_t!");
    }
    Object res = l.eval(m_env);
    return boost::apply_visitor(arithmetic_cast_visitor(m_env), res);
}
}  // namespace varlisp
