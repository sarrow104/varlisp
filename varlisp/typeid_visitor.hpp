#pragma once

#include <boost/variant.hpp>

#include <sss/regex/cregex.hpp>
#include <stdexcept>
#include <iosfwd>

namespace varlisp {
struct Environment;
struct Empty;
struct Nill;
struct symbol;

// 都需要求值; 不过，这就交给程序逻辑吧；
// 这里，还是理论上，用各个int 分开不同的类型；
struct Builtin;
struct Define;
struct IfExpr;
struct Cond;
struct LogicAnd;
struct LogicOr;

struct List;
struct Lambda;
struct gumboNode;

struct typeid_visitor : public boost::static_visitor<int> {
    Environment& m_env;
    explicit typeid_visitor(Environment& env) : m_env(env) {}

    int operator() (const Empty& ) const { throw std::runtime_error("query Empty typeid!"); }
    int operator() (const Nill& ) const { return 0; }
    int operator() (const bool& ) const { return 1; }
    int operator() (const int& ) const { return 2; }
    int operator() (const double& ) const { return 3; }
    int operator() (const std::string& ) const { return 4; }
    int operator() (const sss::regex::CRegex& ) const { return 5; }
    int operator() (const symbol& ) const { return 6; }
    int operator() (const Builtin& ) const { return 7; }
    int operator() (const Define& ) const { return 8; }
    int operator() (const IfExpr& ) const { return 9; }

    int operator() (const Cond& ) const { return 10; }
    int operator() (const LogicAnd& ) const { return 11; }
    int operator() (const LogicOr& ) const { return 12; }
    int operator() (const List& ) const { return 13; }
    int operator() (const Lambda& ) const { return 14; }
    int operator() (const gumboNode& ) const { return 15; }
};
} // namespace varlisp
