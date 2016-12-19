#ifndef __STRICT_LESS_VISITOR_HPP_1457699825__
#define __STRICT_LESS_VISITOR_HPP_1457699825__

#include <boost/variant.hpp>

#include <sss/log.hpp>

#include "arithmetic_t.hpp"
#include "arithmetic_cast_visitor.hpp"
#include "environment.hpp"
#include "object.hpp"

namespace varlisp {
struct strict_less_visitor : boost::static_visitor<bool> {
    varlisp::Environment& m_env;
    strict_less_visitor(varlisp::Environment& env) : m_env(env) {}
    template <typename T, typename U>
    bool operator()(const T& lhs, const U& rhs) const
    {
        try {
            double d1 = arithmetic2double(arithmetic_cast_visitor(m_env)(lhs));
            double d2 = arithmetic2double(arithmetic_cast_visitor(m_env)(rhs));
            return d1 < d2;
        }
        catch(...)
        {
            return false;
        }
    }

    bool operator()(const string_t& lhs, const string_t& rhs) const
    {
        return lhs < rhs;
    }

    bool operator()(Empty lhs, Empty rhs) const
    {
        throw std::runtime_error("Empty < Empty");
    }
    bool operator()(Nill lhs, Nill rhs) const
    {
        throw std::runtime_error("Nill < Nill");
    }

    bool operator()(const varlisp::regex_t& lhs,
                    const varlisp::regex_t& rhs) const
    {
        throw std::runtime_error("regex_t < regex_t");
    }

    template <typename T>
    bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs < rhs;
    }

};
}  // namespace varlisp

#endif /* __STRICT_LESS_VISITOR_HPP_1457699825__ */
