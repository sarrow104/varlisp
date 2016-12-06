#pragma once

#include <boost/variant.hpp>

#include <sss/raw_print.hpp>
#include <sss/regex/cregex.hpp>
#include <sss/raw_print.hpp>

#include "String.hpp"

namespace varlisp {
struct Empty;
struct Nill;
struct symbol;
struct String;
struct keywords_t;
typedef String string_t;
struct Environment;

struct helpmsg_visitor : public boost::static_visitor<varlisp::string_t> {
    Environment& m_env;
    helpmsg_visitor(Environment& env) : m_env(env) {}
    template <typename T>
    varlisp::string_t operator()(const T& ) const
    {
        return varlisp::string_t{"NOT support"};
    }

    varlisp::string_t operator()(const Empty&) const {
        return varlisp::string_t{"empty"};
    }
    varlisp::string_t operator()(const Nill&               ) const { return varlisp::string_t{"nil"};           }
    varlisp::string_t operator()(bool v                    ) const { return varlisp::string_t{v ? "#t" : "#f"}; }
    varlisp::string_t operator()(const string_t&           ) const { return varlisp::string_t{"string"};        }
    varlisp::string_t operator()(const varlisp::symbol& s  ) const ;
    varlisp::string_t operator()(const varlisp::keywords_t& k ) const ;
    varlisp::string_t operator()(const sss::regex::CRegex& ) const { return varlisp::string_t{"regex"};         }
};

}  // namespace varlisp

