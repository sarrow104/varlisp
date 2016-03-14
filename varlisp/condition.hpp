#ifndef __CONDITION_HPP_1457934568__
#define __CONDITION_HPP_1457934568__


#include "object.hpp"

#include <utility>

namespace varlisp {
    struct Environment;

    struct Cond
    {
        std::vector<std::pair<Object, Object> > conditions;
        // (cond (<p1> <e1>)
        //       (<p2> <e2>)
        //       ...
        //       (<pn> <en>))
        //       [(else e)]
        //
        // > (cond )
        // > (cond '())
        // . quote: bad syntax in: quote
        Cond() = default;
        explicit Cond(const std::vector<std::pair<Object, Object> > &conds)
            : conditions(conds)
        {
        }

        explicit Cond(std::vector<std::pair<Object, Object> > &&conds)
            : conditions(std::move(conds))
        {
        }

        Cond(const Cond& rhs) = default;
        Cond& operator=(const Cond& rhs) = default;

        Cond(Cond&& rhs) = default;
        Cond& operator=(Cond&& rhs) = default;

        Object eval(Environment& env) const;

        void print(std::ostream& o) const;
    };

    inline std::ostream& operator << (std::ostream& o, const Cond& d)
    {
        d.print(o);
        return o;
    }
    bool operator==(const Cond& lhs, const Cond& rhs);
    bool operator<(const Cond& lhs, const Cond& rhs);
} // namespace varlisp


#endif /* __CONDITION_HPP_1457934568__ */
