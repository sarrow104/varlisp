#ifndef __CAST2DOUBLE_VISITOR_HPP_1457603964__
#define __CAST2DOUBLE_VISITOR_HPP_1457603964__

#include <sss/utlstring.hpp>

#include "object.hpp"
#include "environment.hpp"

namespace varlisp {
    struct cast2double_visitor : public boost::static_visitor<double>
    {
        Environment & m_env;
        cast2double_visitor(Environment& env)
            : m_env(env)
        {
        }
        template<typename T>
            double operator()(const T& ) const {
                return 0;
            } 

        double operator()(double d) const {
            return d;
        }

        double operator()(int d) const {
            return d;
        }

        double operator()(bool b) const {
            return b ? 1.0 : 0.0;
        }

        double operator()(const std::string& s) const {
            return sss::string_cast_nothrow<double>(s);
        }

        double operator()(const List& l) const {
            Object res = l.eval(m_env);
            // boost::apply_visitor(eval_visitor(m_env), l)
            return boost::apply_visitor(cast2double_visitor(m_env), res);
        }
    };
} // namespace varlisp


#endif /* __CAST2DOUBLE_VISITOR_HPP_1457603964__ */
