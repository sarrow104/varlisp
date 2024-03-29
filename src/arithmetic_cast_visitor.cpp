#include "arithmetic_cast_visitor.hpp"

#include <cstdlib>

#include <sss/raw_print.hpp>

#include "object.hpp"

#include "eval_visitor.hpp"
#include "is_instant_visitor.hpp"

namespace varlisp {

varlisp::arithmetic_t arithmetic_cast_visitor::operator()(
    const std::string& s) const
{
    int64_t iv = 0;
    double dv = 0;
    int offset = -1;
    if (std::sscanf(s.c_str(), "%lld%n", &iv, &offset) == 1 &&
        offset == int64_t(s.length())) {
        return iv;
    }
    if (std::sscanf(s.c_str(), "%lf%n", &dv, &offset) == 1 &&
             offset == int64_t(s.length())) {
        return dv;
    }
    SSS_POSITION_THROW(std::runtime_error, "failed convert ",
                       sss::raw_string(s), " to arithmetic_t");
}

varlisp::arithmetic_t arithmetic_cast_visitor::operator()(const varlisp::symbol& s) const
{
    Object* p_obj = m_env.deep_find(s.name());
    if (p_obj == nullptr) {
        SSS_POSITION_THROW(std::runtime_error, "symbol ", s.name(),
                          " not exists!");
    }
    Object tmp;
    if (boost::apply_visitor(is_instant_visitor(m_env), *p_obj)) {
        return boost::apply_visitor(arithmetic_cast_visitor(m_env), *p_obj);
    }
    tmp = boost::apply_visitor(eval_visitor(m_env), *p_obj);
    return boost::apply_visitor(arithmetic_cast_visitor(m_env), tmp);
}

varlisp::arithmetic_t arithmetic_cast_visitor::operator()(const List& l) const
{
    if (l.is_quoted()) {
        SSS_POSITION_THROW(std::runtime_error,
                          "object: cannot convert from s-list to arithmetic_t!");
    }
    Object res = l.eval(m_env);
    return boost::apply_visitor(arithmetic_cast_visitor(m_env), res);
}

}  // namespace varlisp
