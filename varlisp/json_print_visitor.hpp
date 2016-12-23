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
    struct IndentHelper;
    struct Indent
    {
        friend struct IndentHelper;
        struct endl_t {
            friend std::ostream& operator << (std::ostream&, const endl_t& i);
            bool m_enable;
            explicit endl_t(bool enable)
                : m_enable(enable)
            {
            }
            void print(std::ostream& o) const
            {
                if (m_enable) {
                    o << "\n";
                }
            }
        };
        friend std::ostream& operator << (std::ostream&, const Indent& i);
        explicit Indent(bool enable)
            : m_enable(enable), m_indent(0)
        {}
        void print(std::ostream& o) const;
        bool enable() const {
            return m_enable;
        }
        endl_t endl() const {
            return endl_t(m_enable);
        }
    private:
        bool  m_enable;
        int   m_indent;
    };
    struct IndentHelper
    {
        Indent & m_indent;
        IndentHelper(Indent& indent)
            : m_indent(indent)
        {
            if (m_indent.enable()) {
                ++m_indent.m_indent;
            }
        }
        ~IndentHelper()
        {
            if (m_indent.enable()) {
                --m_indent.m_indent;
            }
        }
    };

    std::ostream& m_o;
    Indent m_indent;
    json_print_visitor(std::ostream& o, bool is_indent = false)
        : m_o(o), m_indent(is_indent)
    {
    }
    json_print_visitor(std::ostream& o, const Indent& indent)
        : m_o(o), m_indent(indent)
    {
    }
    template <typename T>
    void operator()(const T& v) const
    {
        std::ostringstream oss;
        oss << v;
        m_o << sss::raw_string(oss.str());
    }

    void operator()(const Empty&                  ) const { }
    void operator()(const Nill&                   ) const ;
    void operator()(bool v                        ) const ;
    void operator()(int64_t v                     ) const ;
    void operator()(double v                      ) const ;
    void operator()(const varlisp::regex_t& reg   ) const ;
    void operator()(const string_t& v             ) const ;
    void operator()(const varlisp::symbol& s      ) const ;
    void operator()(const varlisp::List& s        ) const ;
    void operator()(const varlisp::Environment& s ) const ;
};

inline std::ostream& operator << (std::ostream& o, const json_print_visitor::Indent& i)
{
    i.print(o);
    return o;
}

inline std::ostream& operator << (std::ostream& o, const json_print_visitor::Indent::endl_t& e)
{
    e.print(o);
    return o;
}

} // namespace varlisp
