#pragma once
// #include "object.hpp"
// #include "gumboNode.hpp"
#include <sss/regex/cregex.hpp>
#include <boost/variant.hpp>

// 即，是否需要eval_visitor()的判断。
// 是否立即值？
namespace varlisp {
struct Environment;
struct symbol;
struct Lambda;
struct List;
struct Empty;
struct Builtin;

class gumboNode;
struct is_instant_visitor : boost::static_visitor<bool> {
    Environment& m_env;
    explicit is_instant_visitor(Environment& env) : m_env(env) {}
    template <typename T>
    bool operator()(const T& ) const
    {
        return false;
    }

    bool operator()(const varlisp::symbol& ) const { return false; }
    bool operator()(const varlisp::List& v) const;

    bool operator()(const varlisp::Lambda& ) const { return true; }
    bool operator()(const sss::regex::CRegex& ) const { return true; }
    bool operator()(const gumboNode& ) const { return true; }
    bool operator()(const Empty&) const { return true; }
    bool operator()(bool ) const { return true; }
    bool operator()(int ) const { return true; }
    bool operator()(double ) const { return true; }
    bool operator()(const std::string ) const { return true; }
    bool operator()(const varlisp::Builtin& ) const { return true; }
};
}  // namespace varlisp

