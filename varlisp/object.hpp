#ifndef __OBJECT_HPP_1457602801__
#define __OBJECT_HPP_1457602801__

#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <iosfwd>

#include <sss/regex/cregex.hpp>

#include "symbol.hpp"
#include "gumboNode.hpp"

namespace varlisp {

struct Empty {
};

inline std::ostream& operator<<(std::ostream& o, const Empty&) { return o; }
inline bool operator==(const Empty&, const Empty&) { return true; }
inline bool operator<(const Empty&, const Empty&) { return false; }

struct Nill {
};

inline std::ostream& operator<<(std::ostream& o, const Nill&) { return o; }
inline bool operator==(const Nill&, const Nill&) { return true; }
inline bool operator<(const Nill&, const Nill&) { return false; }

struct Define;
struct IfExpr;
struct List;
struct Quote;
struct Builtin;
struct Lambda;
struct Cond;
struct LogicAnd;
struct LogicOr;

// NOTE
typedef boost::variant<
    Empty,                                  // 0
    Nill,                                   // 1
    bool,                                   // 2
    int,                                    // 3
    double,                                 // 4
    std::string,                            // 5
    sss::regex::CRegex,                     // 6
    symbol,                                 // 7
    boost::recursive_wrapper<Builtin>,      // 8
    boost::recursive_wrapper<Define>,       // 9
    boost::recursive_wrapper<IfExpr>,       // 10
    boost::recursive_wrapper<Cond>,         // 11
    boost::recursive_wrapper<LogicAnd>,     // 12
    boost::recursive_wrapper<LogicOr>,      // 13
    boost::recursive_wrapper<List>,         // 14
    // NOTE
    // quote-list只是作为一种函数存在！
    boost::recursive_wrapper<Lambda>,       // 15
    gumboNode                               // 16
    >
    Object;
}  // namespace varlisp

#include "Define.hpp"
#include "builtin.hpp"
#include "condition.hpp"
#include "ifexpr.hpp"
#include "lambda.hpp"
#include "list.hpp"
#include "logic_and.hpp"
#include "logic_or.hpp"

#include "print_visitor.hpp"

inline std::ostream& operator<<(std::ostream& o, const varlisp::Object& obj)
{
    boost::apply_visitor(varlisp::print_visitor(o), obj);
    return o;
}

#endif /* __OBJECT_HPP_1457602801__ */
