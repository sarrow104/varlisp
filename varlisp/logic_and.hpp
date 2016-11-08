#ifndef __LOGIC_AND_HPP_1457944383__
#define __LOGIC_AND_HPP_1457944383__

#include "object.hpp"

#include <utility>

namespace varlisp {
struct Environment;

struct LogicAnd {
    std::vector<Object> conditions;
    // (and <e1> ... <en>) ; special form
    // > (and 1)
    // 1
    // > (and)
    // #t

    LogicAnd() = default;
    explicit LogicAnd(const std::vector<Object>& conds) : conditions(conds) {}
    explicit LogicAnd(std::vector<Object>&& conds)
        : conditions(std::move(conds))
    {
    }

    LogicAnd(const LogicAnd& rhs) = default;
    LogicAnd& operator=(const LogicAnd& rhs) = default;

    LogicAnd(LogicAnd&& rhs) = default;
    LogicAnd& operator=(LogicAnd&& rhs) = default;

    Object eval(Environment& env) const;

    void print(std::ostream& o) const;
};

inline std::ostream& operator<<(std::ostream& o, const LogicAnd& d)
{
    d.print(o);
    return o;
}
bool operator==(const LogicAnd& lhs, const LogicAnd& rhs);
bool operator<(const LogicAnd& lhs, const LogicAnd& rhs);
}  // namespace varlisp

#endif /* __LOGIC_AND_HPP_1457944383__ */
