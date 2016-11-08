#ifndef __DEFINE_HPP_1457602918__
#define __DEFINE_HPP_1457602918__

#include "object.hpp"

namespace varlisp {

struct Environment;

struct Define {
    varlisp::symbol name;
    Object value;

    Define() = default;
    Define(const Define&) = default;
    Define& operator=(const Define&) = default;

    Define(const varlisp::symbol& n, const Object& v) : name(n), value(v) {}
    Define(varlisp::symbol&& n, Object&& v)
        : name(std::move(n)), value(std::move(v))
    {
    }

    Define(Define&& rhs) = default;
    Define& operator=(Define&& rhs) = default;

    Object eval(Environment& env) const;

    void print(std::ostream& o) const;
};

inline std::ostream& operator<<(std::ostream& o, const Define& d)
{
    d.print(o);
    return o;
}

bool operator==(const Define& lhs, const Define& rhs);
bool operator<(const Define& lhs, const Define& rhs);

}  // namespace varlisp

#endif /* __DEFINE_HPP_1457602918__ */
