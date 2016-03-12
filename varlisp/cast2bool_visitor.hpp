#ifndef __CAST2BOOL_VISITOR_HPP_1457680829__
#define __CAST2BOOL_VISITOR_HPP_1457680829__

#include <sss/utlstring.hpp>

#include "object.hpp"
#include "environment.hpp"

namespace varlisp {
    struct cast2bool_visitor : public boost::static_visitor<bool>
    {
        Environment & m_env;
        cast2bool_visitor(Environment& env)
            : m_env(env)
        {
        }
        template<typename T>
            bool operator()(const T& ) const {
                return false;
            } 

        bool operator()(double d) const {
            return bool(d);
        }

        bool operator()(int d) const {
            return bool(d);
        }

        bool operator()(bool b) const {
            return b;
        }

        bool operator()(const std::string& s) const {
            return bool(sss::string_cast_nothrow<double>(s));
        }

        bool operator()(const varlisp::symbol& s) const {
            Environment::const_iterator it = m_env.find(s.m_data);
            if (it == m_env.cend()) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "symbol " << s.m_data << " not exists!");
            }
            return boost::apply_visitor(cast2bool_visitor(m_env), it->second);
        }

        bool operator()(const List& l) const {
            return l.length();
            // Object res = l.eval(m_env);
            // return boost::apply_visitor(cast2bool_visitor(m_env), res);
        }
    };
    
} // namespace varlisp


#endif /* __CAST2BOOL_VISITOR_HPP_1457680829__ */
