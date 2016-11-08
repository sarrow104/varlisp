#ifndef __LOGIC_OR_HPP_1457944790__
#define __LOGIC_OR_HPP_1457944790__

#include "object.hpp"

#include <utility>

namespace varlisp {
struct Environment;

struct LogicOr {
    std::vector<Object> conditions;
    // (and <e1> ... <en>) ; special form
    // > (and 1)
    // 1
    // > (and)
    // #t

    LogicOr() = default;
    explicit LogicOr(const std::vector<Object>& conds) : conditions(conds) {}
    explicit LogicOr(std::vector<Object>&& conds) : conditions(std::move(conds))
    {
    }

    LogicOr(const LogicOr& rhs) = default;
    LogicOr& operator=(const LogicOr& rhs) = default;

    LogicOr(LogicOr&& rhs) = default;
    LogicOr& operator=(LogicOr&& rhs) = default;

    Object eval(Environment& env) const;

    void print(std::ostream& o) const;
};

inline std::ostream& operator<<(std::ostream& o, const LogicOr& d)
{
    d.print(o);
    return o;
}
bool operator==(const LogicOr& lhs, const LogicOr& rhs);
bool operator<(const LogicOr& lhs, const LogicOr& rhs);
}  // namespace varlisp

#endif /* __LOGIC_OR_HPP_1457944790__ */
