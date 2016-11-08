#ifndef __CAST2DOUBLE_VISITOR_HPP_1457603964__
#define __CAST2DOUBLE_VISITOR_HPP_1457603964__

#include <sss/util/PostionThrow.hpp>
#include <sss/utlstring.hpp>

#include "environment.hpp"
#include "object.hpp"

namespace varlisp {
struct cast2double_visitor : public boost::static_visitor<double> {
    Environment& m_env;
    cast2double_visitor(Environment& env) : m_env(env) {}
    template <typename T>
    double operator()(const T& v) const
    {
        SSS_POSTION_THROW(std::runtime_error,
                          "object: cannot convert to double!");
    }

    double operator()(double d) const { return d; }
    double operator()(int d) const { return d; }
    double operator()(bool b) const { return b ? 1.0 : 0.0; }
    double operator()(const std::string& s) const
    {
        return sss::string_cast_nothrow<double>(s);
    }

    double operator()(const varlisp::symbol& s) const
    {
        Object* p_obj = m_env.find(s.m_data);
        if (!p_obj) {
            SSS_POSTION_THROW(std::runtime_error, "symbol ", s.m_data,
                              " not exists!");
        }
        return boost::apply_visitor(cast2double_visitor(m_env), *p_obj);
    }

    double operator()(const List& l) const
    {
        Object res = l.eval(m_env);
        return boost::apply_visitor(cast2double_visitor(m_env), res);
    }
};
}  // namespace varlisp

#endif /* __CAST2DOUBLE_VISITOR_HPP_1457603964__ */
