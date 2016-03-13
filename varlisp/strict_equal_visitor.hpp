#ifndef __STRICT_EQUAL_VISITOR_HPP_1457603635__
#define __STRICT_EQUAL_VISITOR_HPP_1457603635__

#include <boost/variant.hpp>

#include "environment.hpp"
#include "cast2double_visitor.hpp"

namespace varlisp {
    struct strict_equal_visitor : boost::static_visitor<bool>
    {
        Environment& m_env;
        explicit strict_equal_visitor(Environment& env)
            : m_env(env)
        {
        }
        template <typename T, typename U>
            bool operator() (const T& lhs, const U& rhs) const
            {
                double d1 = cast2double_visitor(m_env)(lhs);
                double d2 = cast2double_visitor(m_env)(rhs);
                return d1 == d2;
            }

        template <typename T>
            bool operator() (const T& lhs, const T& rhs) const
            {
                return lhs == rhs;
            }

        bool operator() (Empty lhs, Empty rhs) const
        {
            throw std::runtime_error("Empty = Empty");
        }
    };
} // namespace varlisp


#endif /* __STRICT_EQUAL_VISITOR_HPP_1457603635__ */
