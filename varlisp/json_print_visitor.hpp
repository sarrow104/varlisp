#pragma once

#include <boost/variant.hpp>

#include <sss/raw_print.hpp>

#include "regex_t.hpp"

namespace varlisp {
struct Empty;
struct Nill;
struct symbol;
struct List;
struct String;
struct Environment;
typedef String string_t;

struct json_print_visitor : public boost::static_visitor<void> {
    std::ostream& m_o;
    json_print_visitor(std::ostream& o) : m_o(o) {}
    template <typename T>
    void operator()(const T& v) const
    {
        std::ostringstream oss;
        oss << v;
        m_o << sss::raw_string(oss.str());
    }

    void operator()(const Empty&                  ) const {                                }
    void operator()(const Nill&                   ) const { m_o << "null";                 }
    void operator()(bool v                        ) const { m_o << (v ? "true" : "false"); }
    void operator()(int64_t v                     ) const { m_o << v;                      }
    void operator()(double v                      ) const { m_o << v;                      }
    void operator()(const varlisp::regex_t& reg   ) const ;
    void operator()(const string_t& v             ) const ;
    void operator()(const varlisp::symbol& s      ) const ;
    void operator()(const varlisp::List& s        ) const ;
    void operator()(const varlisp::Environment& s ) const ;
};

} // namespace varlisp
