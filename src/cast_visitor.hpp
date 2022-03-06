#pragma once

#include <limits>
#include <sstream>

#include <boost/variant.hpp>

#include <sss/utlstring.hpp>

#include "builtin_helper.hpp"
#include "cast2bool_visitor.hpp"
#include "detail/is_symbol.hpp"
#include "environment.hpp"
#include "object.hpp"
#include "print_visitor.hpp"

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
    Object operator() (const T&  /*unused*/, const U&  /*unused*/) const
    {
        return Nill{};
    }

    // (cast a "my-var") -> my-var
    // (cast a 'var-b) -> var-b
    //
    // NOTE
    //
    // (eval 'a) -> cannot find symbol a | nil (newlisp中，未定义的符号值是nil）
    template<typename T>
    Object operator() (const symbol& /*unused*/, const T&  /*v*/) const
    {
        Object temp;
        const Object& res = varlisp::getAtomicValueUnquote(m_env, m_value, temp);

        if (const auto *p_sym = boost::get<varlisp::symbol>(&res)) {
            return res;
        }
        if (const auto *p_string = boost::get<varlisp::string_t>(&res)) {
            auto name = p_string->gen_shared();
            if (detail::is_symbol(*name)) {
                return symbol(*name);
            }
        }
        return Nill{};
    }
    template<typename T>
    Object operator() (const int64_t& /*unused*/, const T&  /*v*/) const
    {
        Object temp;
        const Object& res = varlisp::getAtomicValueUnquote(m_env, m_value, temp);
        if (const auto * p_double = boost::get<double>(&res)) {
            if (std::abs(*p_double) > double(std::numeric_limits<int64_t>::max())) {
                return Nill{};
            }
            return int64_t(*p_double);
        }
        if (const auto * p_int64 = boost::get<int64_t>(&res)) {
            return *p_int64;
        }
        if (const auto * p_string = boost::get<varlisp::string_t>(&res)) {
            auto s = p_string->gen_shared();
            try {
                return sss::string_cast<int64_t>(*s);
            }
            catch(...) {
                return Nill{};
            }
        }
        return Nill{};
    }
    template<typename T>
    Object operator() (const double& /*unused*/, const int64_t&  /*v*/) const
    {
        Object temp;
        const Object& res = varlisp::getAtomicValueUnquote(m_env, m_value, temp);
        if (const auto * p_bool = boost::get<bool>(&res)) {
            return double(*p_bool);
        }
        if (const auto * p_double = boost::get<double>(&res)) {
            return *p_double;
        }
        if (const auto * p_int64 = boost::get<int64_t>(&res)) {
            return double(*p_int64);
        }
        if (const auto * p_string = boost::get<varlisp::string_t>(&res)) {
            auto s = p_string->gen_shared();
            try {
                return sss::string_cast<double>(*s);
            }
            catch(...) {
                return Nill{};
            }
        }
        return Nill{};
    }
    template<typename T>
    Object operator() (const string_t& /*unused*/, const T&  /*value*/) const
    {
        Object temp;
        const Object& res = varlisp::getAtomicValueUnquote(m_env, m_value, temp);
        if (const auto * p_string = boost::get<varlisp::string_t>(&res)) {
            return res;
        }
        if (const auto * p_sym = boost::get<varlisp::symbol>(&res)) {
            return string_t(p_sym->name());
        }
        std::ostringstream oss;
        boost::apply_visitor(print_visitor(oss), m_value);
        return string_t{oss.str()};
    }
    template<typename T>
    Object operator() (const bool& /*unused*/, const T&  /*s*/) const
    {
        return boost::apply_visitor(cast2bool_visitor(m_env), m_value);
    }
    //NOTE cast到正则
    Object operator() (const varlisp::regex_t& /*unused*/, const varlisp::string_t& s) const
    {
        return std::make_shared<RE2>(*s.gen_shared());
    }
    // NOTE
    // 并不准备支持cast到gumbo-node
    // 因为，我已经添加了 // 的方式，来语法糖方式生成正则对象；
    // 但是，gumbo-node，就不方便使用语法糖了——干扰解析器工作。
    // 
    // "//" 相当于 (regex "") 的结果。
};
} // namespace varlisp
