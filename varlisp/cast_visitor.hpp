#pragma once

#include <limits>
#include <sstream>

#include <boost/variant.hpp>

#include <sss/utlstring.hpp>

#include "object.hpp"
#include "environment.hpp"
#include "print_visitor.hpp"
#include "cast2bool_visitor.hpp"
#include "detail/is_symbol.hpp"

namespace varlisp {

struct cast_visitor : public boost::static_visitor<Object>
{
    Environment& m_env;
    const Object& m_target;
    const Object& m_value;
    explicit cast_visitor(Environment& env,
                          const Object& target,
                          const Object& value)
        : m_env(env),
          m_target(target),
          m_value(value)
    {}

    template <typename T, typename U>
    Object operator() (const T& , const U& ) const
    {
        return Nill{};
    }

    Object operator() (const symbol&, const string_t& s) const
    {
        std::string name = s.to_string_view().to_string();
        if (detail::is_symbol(name)) {
            return symbol(name);
        }
        else {
            return Nill{};
        }
    }
    Object operator() (const int&, const double& f) const {
        if (std::abs(f) > std::numeric_limits<int>::max()) {
            return Nill{};
        }
        return int(f);
    }
    Object operator() (const double&, const int& f) const {
        return double(f);
    }
    template<typename T>
    Object operator() (const string_t, const T& value) const {
        std::ostringstream oss;
        boost::apply_visitor(print_visitor(oss), m_value);
        return string_t{std::move(oss.str())};
    }
    Object operator() (const int&, const string_t& s) const {
        sss::string_view sv = s.to_string_view();
        try {
            return sss::string_cast<int>(sv.to_string());
        }
        catch(...) {
            return Nill{};
        }
    }
    Object operator() (const double&, const string_t& s) const {
        sss::string_view sv = s.to_string_view();
        try {
            return sss::string_cast<double>(sv.to_string());
        }
        catch(...) {
            return Nill{};
        }
    }
    template<typename T>
    Object operator() (const bool&, const T& s) const {
        return boost::apply_visitor(cast2bool_visitor(m_env), m_value);
    }
    //NOTE cast到正则
    Object operator() (const sss::regex::CRegex&, const varlisp::string_t& s) const {
        return sss::regex::CRegex(s.to_string_view().to_string());
    }
    // NOTE
    // 并不准备支持cast到gumbo-node
    // 因为，我已经添加了 // 的方式，来语法糖方式生成正则对象；
    // 但是，gumbo-node，就不方便使用语法糖了——干扰解析器工作。
    // 
    // "//" 相当于 (regex "") 的结果。
};
} // namespace varlisp
