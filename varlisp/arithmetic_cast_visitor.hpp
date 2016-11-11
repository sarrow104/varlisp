#pragma once

#include <sss/util/PostionThrow.hpp>
#include <sss/utlstring.hpp>

#include "environment.hpp"
#include "arithmetic_t.hpp"

namespace varlisp {
struct Environment;
struct symbol;
struct List;
struct arithmetic_cast_visitor : public boost::static_visitor<varlisp::arithmetic_t> {
    Environment& m_env;
    arithmetic_cast_visitor(Environment& env) : m_env(env) {}
    template <typename T>
    varlisp::arithmetic_t operator()(const T& v) const
    {
        SSS_POSTION_THROW(std::runtime_error,
                          "object: cannot convert ", v, "to double!");
    }

    varlisp::arithmetic_t operator()(double d) const { return d; }
    varlisp::arithmetic_t operator()(int d) const { return d; }
    varlisp::arithmetic_t operator()(bool b) const { return b ? 1.0 : 0.0; }
    varlisp::arithmetic_t operator()(const std::string& s) const;

    varlisp::arithmetic_t operator()(const varlisp::symbol& s) const;

    varlisp::arithmetic_t operator()(const List& l) const;
};
} // namespace varlisp
