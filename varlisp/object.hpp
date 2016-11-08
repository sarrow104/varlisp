#ifndef __OBJECT_HPP_1457602801__
#define __OBJECT_HPP_1457602801__

#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <iosfwd>

#include <sss/regex/cregex.hpp>

#include "symbol.hpp"

namespace varlisp {

struct Empty {
};

inline std::ostream& operator<<(std::ostream& o, const Empty&) { return o; }
inline bool operator==(const Empty&, const Empty&) { return true; }
inline bool operator<(const Empty&, const Empty&) { return false; }
struct Define;
struct IfExpr;
struct List;
struct Quote;
struct Builtin;
struct Lambda;
struct Cond;
struct LogicAnd;
struct LogicOr;

typedef boost::variant<
    Empty,                               // 0
    bool,                                // 1
    int,                                 // 2
    double,                              // 3
    std::string,                         // 4
    sss::regex::CRegex,                  // 5
    symbol,                              // 6
    boost::recursive_wrapper<Builtin>,   // 7
    boost::recursive_wrapper<Define>,    // 8
    boost::recursive_wrapper<IfExpr>,    // 9
    boost::recursive_wrapper<Cond>,      // 10
    boost::recursive_wrapper<LogicAnd>,  // 11
    boost::recursive_wrapper<LogicOr>,   // 12
    boost::recursive_wrapper<List>,      // 13  // NOTE
                                     // quote-list只是作为一种函数存在！
    boost::recursive_wrapper<Lambda>  // 14
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

std::ostream& operator<<(std::ostream& o, const varlisp::Object& obj);

#endif /* __OBJECT_HPP_1457602801__ */
