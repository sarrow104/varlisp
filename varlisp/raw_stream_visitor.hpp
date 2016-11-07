#ifndef __RAW_STREAM_VISITOR_HPP_1478481280__
#define __RAW_STREAM_VISITOR_HPP_1478481280__

#include <boost/variant.hpp>

#include "object.hpp"

namespace varlisp {

struct raw_stream_visitor : public boost::static_visitor<void> {
    // 这个只是负责打印，不应该执行！
    // 但是，需要获取变量的值；所以：
    std::ostream& m_o;
    varlisp::Environment& m_env;
    raw_stream_visitor(std::ostream& o, varlisp::Environment& env) : m_o(o), m_env(env) {}
    template <typename T>
    void operator()(const T& v) const
    {
        m_o << v;
    }

    void operator()(const Empty&) const {}
    void operator()(bool v) const { m_o << (v ? "true" : "false"); }
    void operator()(const std::string& v) const { m_o << v; }
    void operator()(const varlisp::symbol& s) const;
    void operator()(const sss::regex::CRegex& reg) const { m_o << reg.regstr(); }
};

}  // namespace varlisp

#endif /* __RAW_STREAM_VISITOR_HPP_1478481280__ */
