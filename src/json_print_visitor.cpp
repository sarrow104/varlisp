#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "json_print_visitor.hpp"

#include "String.hpp"
#include "symbol.hpp"
#include "list.hpp"
#include "environment.hpp"
#include "print_visitor.hpp"
#include "detail/list_iterator.hpp"
#include "detail/json.hpp"

namespace varlisp {

void json_print_visitor::Indent::print(std::ostream& o) const {
    if (m_enable) {
        for (int i = 0; i < m_indent; ++i) {
            o << varlisp::detail::json::get_json_indent();
        }
    }
}

void json_print_visitor::operator()(const Nill& ) const
{
    m_o << "null";
}

void json_print_visitor::operator()(bool v ) const
{
    m_o << (v ? "true" : "false");
}
void json_print_visitor::operator()(int64_t v) const
{
    m_o << v;          
}

void json_print_visitor::operator()(double v) const
{
    m_o << v;
}

void json_print_visitor::operator()(const varlisp::regex_t& reg) const
{
    // FIXME 应该escape一下
    if (!reg) {
        SSS_POSITION_THROW(std::runtime_error, "null regex-obj");
    }
    m_o << '/' << reg->pattern() << '/';
}

void json_print_visitor::operator()(const string_t& v) const
{
    m_o << sss::raw_string(v);
}

void json_print_visitor::operator()(const varlisp::symbol& s) const
{
    // 貌似，也应该按字符串输出！
    m_o << sss::raw_string(s.name());
}

void json_print_visitor::operator()(const varlisp::List& s) const
{
    const List * p_tail = &s;
    if (s.is_quoted()) {
        p_tail = boost::get<varlisp::List>(&s.nth(1));
        if (!p_tail) {
            // (quote 字面值)
            boost::apply_visitor(json_print_visitor(m_o, m_indent), s.nth(1));
        }
    }
    if (p_tail) {
        m_o << '[';
        if (p_tail->size()) {
            Indent inner(m_indent);
            IndentHelper ind(inner);
            bool is_first = true;
            for (auto it = p_tail->begin(); it != p_tail->end(); ++it) {
                if (is_first) {
                    is_first = false;
                }
                else {
                    m_o << ",";
                }
                m_o << m_indent.endl() << inner;
                boost::apply_visitor(json_print_visitor(m_o, inner), *it);
            }
            m_o << m_indent.endl() << m_indent;
        }
        m_o << ']';
    }
    // // NOTE 针对无法与json格式一一对应的类型，全部转换为字符串。
    // std::ostringstream oss;
    // s.print(oss);
    // COLOG_ERROR(oss.str());
    // COLOG_ERROR(sss::raw_string(oss.str()));
    // m_o << sss::raw_string(oss.str());
}

void json_print_visitor::operator()(const varlisp::Environment& s) const
{
    m_o << '{';
    if (!s.empty()) {
        bool is_first = true;
        Indent inner(m_indent);
        IndentHelper ind(inner);
        for (auto it = s.begin(); it != s.end(); ++it) {
            if (is_first) {
                is_first = false;
            }
            else {
                m_o << ",";
            }
            m_o << m_indent.endl() << inner;
            m_o << sss::raw_string(it->first) << ":";
            if (m_indent.enable()) {
                m_o <<" ";
            }
            boost::apply_visitor(json_print_visitor(m_o, inner), it->second.first);
        }
        m_o << m_indent.endl() << m_indent;
    }
    m_o << '}';
}

} // namespace varlisp
