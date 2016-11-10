#ifndef __SYMBOL_HPP_1457614146__
#define __SYMBOL_HPP_1457614146__

#include <string>

#include <iostream>

namespace varlisp {
struct symbol {
    symbol() = default;
    explicit symbol(const std::string& data) : m_data(data) {}
    std::string m_data;

    bool is_nil() const {
        return this->m_data == "nil";
    }
    void print(std::ostream& o) const { o << this->m_data; }
    bool operator==(const symbol& ref) const
    {
        return this == &ref || this->m_data == ref.m_data;
    }

    bool operator<(const symbol& ref) const
    {
        return this != &ref && this->m_data < ref.m_data;
    }
};

inline std::ostream& operator<<(std::ostream& o, const varlisp::symbol& s)
{
    s.print(o);
    return o;
}

}  // namespace varlisp

#endif /* __SYMBOL_HPP_1457614146__ */
