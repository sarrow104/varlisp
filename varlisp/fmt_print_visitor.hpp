#include <boost/variant.hpp>
#include <iosfwd>
#include <sstream>

#include <sss/regex/cregex.hpp>
#include "fmtArgInfo.hpp"

namespace varlisp {
struct fmtArgInfo;
struct Empty;
struct Nill;
struct String;
typedef String string_t;
struct symbol;
struct fmt_print_visitor : public boost::static_visitor<void> {
    std::ostream& m_o;
    const fmtArgInfo& m_fmt;
    fmt_print_visitor(std::ostream& o, const fmtArgInfo& fmt)
        : m_o(o), m_fmt(fmt)
    {
    }
    ~fmt_print_visitor() {}
    template <typename T>
    void operator()(const T& v) const
    {
        std::ostringstream oss;
        oss << v;
        m_fmt.print(m_o, oss.str());
    }

    void operator()(const Empty&) const {}
    void operator()(const Nill&) const { m_fmt.print(m_o, "nil"); }
    void operator()(bool v) const { m_fmt.print(m_o, v); }
    void operator()(int v) const { m_fmt.print(m_o, v); }
    void operator()(double v) const { m_fmt.print(m_o, v); }
    void operator()(const string_t& v) const;
    void operator()(const varlisp::symbol& s) const;
    void operator()(const sss::regex::CRegex& reg) const
    {
        std::string tmp;
        tmp.reserve(reg.regstr().length() + 2);
        tmp += '/';
        tmp += reg.regstr();
        tmp += '/';
        m_fmt.print(m_o, tmp);
    }
};

}  // namespace varlisp
