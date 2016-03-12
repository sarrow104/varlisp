#ifndef __IFEXPR_HPP_1457602973__
#define __IFEXPR_HPP_1457602973__

#include "object.hpp"

#include <utility>

namespace varlisp {
    struct Environment;

    struct IfExpr
    {
        IfExpr() = default;
        IfExpr(const Object& con, const Object& conse, const Object& alter )
            : condition(con), consequent(conse), alternative(alter)
        {
        }

        IfExpr(const Object&& con, const Object&& conse, const Object&& alter )
            : condition(std::move(con)), consequent(std::move(conse)), alternative(std::move(alter))
        {
        }

        IfExpr(const IfExpr& rhs) = default;
        IfExpr& operator=(const IfExpr& rhs) = default;

        IfExpr(IfExpr&& rhs) = default;
        IfExpr& operator=(IfExpr&& rhs) = default;

        Object eval(Environment& env) const;

        Object condition;
        Object consequent;
        Object alternative;
        void print(std::ostream& o) const;
    };

    inline std::ostream& operator << (std::ostream& o, const IfExpr& d)
    {
        d.print(o);
        return o;
    }
    bool operator==(const IfExpr& lhs, const IfExpr& rhs);
    bool operator<(const IfExpr& lhs, const IfExpr& rhs);
} // namespace varlisp


#endif /* __IFEXPR_HPP_1457602973__ */
