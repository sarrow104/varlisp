#ifndef __OBJECT_HPP_1457602801__
#define __OBJECT_HPP_1457602801__

#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>

#include "symbol.hpp"

namespace varlisp {
    
struct Empty {};

inline bool operator==(const Empty& , const Empty& )
{
    return true;
}

struct Define;
struct IfExpr;
struct List;
struct Quote;
struct Lambda;

typedef boost::variant<
        Empty,              // 0
        bool,               // 1
        int,                // 2
        double,             // 3
        std::string,        // 4
        symbol,             // 5
        boost::recursive_wrapper<Define>,   // 6
        boost::recursive_wrapper<IfExpr>,   // 7
        boost::recursive_wrapper<List>,     // 8
        boost::recursive_wrapper<Quote>,    // 9
        boost::recursive_wrapper<Lambda>    // 10
        > Object;
} // namespace varlisp

#include "Define.hpp"
#include "ifexpr.hpp"
#include "list.hpp"
#include "quote.hpp"
#include "lambda.hpp"

#endif /* __OBJECT_HPP_1457602801__ */
