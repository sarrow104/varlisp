#include "print_visitor.hpp"

#include "String.hpp"
#include "symbol.hpp"

namespace varlisp {
void print_visitor::operator()(const string_t& v) const { m_o << sss::raw_string(v); }

void print_visitor::operator()(const varlisp::symbol& s) const { m_o << s.m_data; }
} // namespace varlisp
