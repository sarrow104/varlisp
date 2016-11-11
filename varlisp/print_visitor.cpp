#include "print_visitor.hpp"

#include "symbol.hpp"

namespace varlisp {

void print_visitor::operator()(const varlisp::symbol& s) const { m_o << s.m_data; }
} // namespace varlisp
