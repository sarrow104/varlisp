#ifndef __OBJECT_HPP_1457602801__
#define __OBJECT_HPP_1457602801__

#include <iosfwd>
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>

#include "symbol.hpp"

namespace varlisp {
    
struct Empty {};

inline bool operator==(const Empty& , const Empty& )
{
    return true;
}

inline bool operator<(const Empty& , const Empty& )
{
    return false;
}

struct Define;
struct IfExpr;
struct List;
struct Quote;
struct Builtin;
struct Lambda;

typedef boost::variant<
        Empty,              // 0
        bool,               // 1
        int,                // 2
        double,             // 3
        std::string,        // 4
        symbol,             // 5
        boost::recursive_wrapper<Builtin>,  // 6
        boost::recursive_wrapper<Define>,   // 7
        boost::recursive_wrapper<IfExpr>,   // 8
        boost::recursive_wrapper<List>,     // 9  // NOTE quote-list只是作为一种函数存在！
        boost::recursive_wrapper<Lambda>    // 10
        > Object;
} // namespace varlisp

#include "Define.hpp"
#include "ifexpr.hpp"
#include "list.hpp"
#include "builtin.hpp"
#include "lambda.hpp"

std::ostream& operator << (std::ostream& o, const varlisp::Object& obj);

#endif /* __OBJECT_HPP_1457602801__ */
