#ifndef __STRICT_LESS_VISITOR_HPP_1457699825__
#define __STRICT_LESS_VISITOR_HPP_1457699825__

#include <boost/variant.hpp>

#include "cast2double_visitor.hpp"
#include "environment.hpp"

namespace varlisp {
    struct strict_less_visitor : boost::static_visitor<bool>
    {
        varlisp::Environment& m_env;
        strict_less_visitor(varlisp::Environment& env)
            : m_env(env)
        {
        }
        template <typename T, typename U>
            bool operator() (const T& lhs, const U& rhs) const
            {
                double d1 = boost::apply_visitor(cast2double_visitor(m_env), lhs);
                double d2 = boost::apply_visitor(cast2double_visitor(m_env), rhs);
                return d1 < d2;
            }

        bool operator() (const std::string& lhs, const std::string& rhs) const
        {
            return lhs < rhs;
        }

        bool operator() (const Empty& lhs, const Empty& rhs) const
        {
            throw std::runtime_error("Empty < Empty");
        }

        // template <typename T>
        //     bool operator() (const T& lhs, const T& rhs) const
        //     {
        //         return lhs < rhs;
        //     }

        // template<typename T>
        //     bool operator() (const T& lhs, const )

        // // 如何解决提升？
        // //
        // // 左右交换的话，可以利用转发；
        // bool operator()
    };
} // namespace varlisp



#endif /* __STRICT_LESS_VISITOR_HPP_1457699825__ */

