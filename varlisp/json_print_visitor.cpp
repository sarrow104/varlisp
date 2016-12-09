#include "json_print_visitor.hpp"

#include "String.hpp"
#include "symbol.hpp"
#include "list.hpp"
#include "environment.hpp"
#include "print_visitor.hpp"
#include "detail/list_iterator.hpp"

namespace varlisp {

void json_print_visitor::operator()(const sss::regex::CRegex& reg) const
{
    // FIXME 应该escape一下
    m_o << '/' << reg.regstr() << '/';
}

void json_print_visitor::operator()(const string_t& v) const { m_o << sss::raw_string(v); }
void json_print_visitor::operator()(const varlisp::symbol& s) const { m_o << s.m_data; }
void json_print_visitor::operator()(const varlisp::List& s) const
{
    if (s.is_squote()) {
        const List * p_tail = nullptr;
        p_tail = boost::get<varlisp::List>(&s.nth(1));
        if (p_tail) {
            m_o << '[';
            bool is_first = true;
            for (auto it = p_tail->begin(); it != p_tail->end(); ++it) {
                if (is_first) {
                    is_first = false;
                }
                else {
                    m_o << ", ";
                }
                boost::apply_visitor(json_print_visitor(m_o), *it);
            }
            m_o << ']';
        }
        else {
            // (quote 字面值)
            boost::apply_visitor(json_print_visitor(m_o), s.nth(1));
        }
    }
    else {
        std::ostringstream oss;
        s.print(oss);
        COLOG_ERROR(oss.str());
        COLOG_ERROR(sss::raw_string(oss.str()));
        m_o << sss::raw_string(oss.str());
    }
}

void json_print_visitor::operator()(const varlisp::Environment& s) const
{
    m_o << '{';
    bool is_first = true;
    for (auto it = s.begin(); it != s.end(); ++it) {
        if (!is_first) {
            m_o << ",";
        }
        else {
            is_first = false;
        }
        m_o << sss::raw_string(it->first);
        m_o << ":";
        boost::apply_visitor(json_print_visitor(m_o), it->second);
    }
    m_o << '}';
}

} // namespace varlisp
