#ifndef __PRINT_VISITOR_HPP_1457603502__
#define __PRINT_VISITOR_HPP_1457603502__

#include <stdexcept>

#include <boost/variant.hpp>

#include <sss/raw_print.hpp>
#include <sss/util/PostionThrow.hpp>

#include "regex_t.hpp"

namespace varlisp {
struct Empty;
struct Nill;
struct symbol;
struct String;
using string_t = String;

struct print_visitor : public boost::static_visitor<void> {
private:
    std::ostream& m_o;

public:
    explicit print_visitor(std::ostream& o) : m_o(o) {}
    template <typename T>
    void operator()(const T& v) const
    {
        m_o << v;
    }

    void operator()(const Empty&                   /*unused*/) const {}
    void operator()(const Nill&                    /*unused*/) const { m_o << "nil";             }
    void operator()(bool v                        ) const { m_o << (v ? "#t" : "#f"); }
    void operator()(const string_t& v             ) const ;
    void operator()(const varlisp::symbol& s      ) const ;
    void operator()(const varlisp::regex_t& reg   ) const
    {
        if (!reg) {
            SSS_POSITION_THROW(std::runtime_error,
                               "nullptr regex-obj");
        }
        m_o << '/' << reg->pattern() << '/';
    }
};

}  // namespace varlisp

#endif /* __PRINT_VISITOR_HPP_1457603502__ */
