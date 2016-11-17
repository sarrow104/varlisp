#include "fmt_print_visitor.hpp"
#include "String.hpp"
#include "symbol.hpp"

namespace varlisp {

void fmt_print_visitor::operator()(const varlisp::symbol& s) const
{
    m_fmt.print(m_o, s.m_data);
}
void fmt_print_visitor::operator()(const string_t& v) const
{
    m_fmt.print(m_o, v.to_string_view());
}
}  // namespace varlisp