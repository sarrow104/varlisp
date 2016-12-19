#include <iosfwd>
#include <sstream>

#include <boost/variant.hpp>

#include <sss/colorlog.hpp>

#include "regex_t.hpp"
#include "fmtArgInfo.hpp"

namespace varlisp {
struct fmtArgInfo;
struct Empty;
struct Nill;
struct String;
typedef String string_t;
struct symbol;
struct keywords_t;
struct List;
struct Environment;
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
    void operator()(const Nill&) const {
        // why must explicit using sss::string_view?
        // or will call m_fmt.print<bool>()
        m_fmt.print(m_o, sss::string_view("nil"));
    }
    void operator()(bool    v) const { m_fmt.print(m_o, v); }
    void operator()(int64_t v) const { m_fmt.print(m_o, v); }
    void operator()(double  v) const { m_fmt.print(m_o, v); }
    void operator()(const string_t&             v) const;
    void operator()(const varlisp::keywords_t&  s) const;
    void operator()(const varlisp::symbol&      s) const;
    void operator()(const varlisp::List&        l) const;
    void operator()(const varlisp::Environment& e) const;
    void operator()(const varlisp::regex_t& reg) const
    {
        std::string tmp;
#ifdef USE_SSS_CREGEX
        tmp.reserve(reg.regstr().length() + 2);
#else
        if (reg) {
            tmp.reserve(reg->pattern().length() + 2);
        }
#endif
        tmp += '/';
#ifdef USE_SSS_CREGEX
        tmp += reg.regstr();
#else
        if (reg) {
            tmp += reg->pattern();
        }
#endif
        tmp += '/';
        m_fmt.print(m_o, tmp);
    }
};

}  // namespace varlisp
