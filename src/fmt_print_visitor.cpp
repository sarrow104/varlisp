#include "fmt_print_visitor.hpp"

#include "String.hpp"
#include "keyword_t.hpp"
#include "symbol.hpp"

namespace varlisp {

void fmt_print_visitor::operator()(const varlisp::symbol& s) const
{
    m_fmt.print(m_o, s.name());
}
void fmt_print_visitor::operator()(const varlisp::keywords_t&  s) const
{
    m_fmt.print(m_o, s.name());
}
void fmt_print_visitor::operator()(const string_t& v) const
{
    m_fmt.print(m_o, v.to_string_view());
}
void fmt_print_visitor::operator()(const varlisp::List&        l) const
{
    m_fmt.print(m_o, l);
}
void fmt_print_visitor::operator()(const varlisp::Environment& e) const
{
    m_fmt.print(m_o, e);
}

}  // namespace varlisp
