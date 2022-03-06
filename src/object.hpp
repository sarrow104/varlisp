#ifndef __OBJECT_HPP_1457602801__
#define __OBJECT_HPP_1457602801__

#include <cstddef>
#include <iosfwd>

#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>

#include "String.hpp"
#include "gumboNode.hpp"
#include "keyword_t.hpp"
#include "regex_t.hpp"
#include "symbol.hpp"

namespace varlisp {

struct Empty {
};

inline std::ostream& operator<<(std::ostream& o, const Empty& /*unused*/) { return o; }
inline bool operator==(const Empty& /*unused*/, const Empty& /*unused*/) { return true; }
inline bool operator<(const Empty& /*unused*/, const Empty& /*unused*/) { return false; }

struct Nill {
};

inline std::ostream& operator<<(std::ostream& o, const Nill& /*unused*/) {
    o << "nil";
    return o;
}
inline bool operator==(const Nill& /*unused*/, const Nill& /*unused*/) { return true; }
inline bool operator<(const Nill& /*unused*/, const Nill& /*unused*/) { return false; }

struct Define;
struct IfExpr;
struct List;
struct Quote;
struct Builtin;
struct Lambda;
struct Cond;
struct LogicAnd;
struct LogicOr;
struct Environment;

// struct String;
using string_t = ::varlisp::String;
// typedef ::std::string string_t;

// NOTE
using Object = boost::variant<
    Empty,                                  // 0
    Nill,                                   // 1
    bool,                                   // 2
    int64_t,                                // 3
    double,                                 // 4
    string_t,                               // 5
    regex_t,                                // 6
    symbol,                                 // 7
    keywords_t,                             // 8
    gumboNode,                              // 9
    boost::recursive_wrapper<Builtin>,      // 10
    boost::recursive_wrapper<Define>,       // 11
    boost::recursive_wrapper<IfExpr>,       // 12
    boost::recursive_wrapper<Cond>,         // 13
    boost::recursive_wrapper<LogicAnd>,     // 14
    boost::recursive_wrapper<LogicOr>,      // 15
    boost::recursive_wrapper<List>,         // 16
    // NOTE
    // quote-list只是作为一种函数存在！
    boost::recursive_wrapper<Lambda>,       // 17
    boost::recursive_wrapper<Environment>   // 18
    >;

Object apply(Environment& env, const Object& funcObj, const List& args);

}  // namespace varlisp

#include "Define.hpp"
#include "builtin.hpp"
#include "condition.hpp"
#include "environment.hpp"
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
