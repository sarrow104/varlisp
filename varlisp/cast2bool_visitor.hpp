#ifndef __CAST2BOOL_VISITOR_HPP_1457680829__
#define __CAST2BOOL_VISITOR_HPP_1457680829__

#include <boost/variant.hpp>

#include <sss/utlstring.hpp>

// NOTE
// 本visitor功能，和
// varlisp/builtin_helper.hpp:57 is_true()
// 有些重复
namespace varlisp {
struct Environment;
struct symbol;
struct List;
struct String;
typedef varlisp::String string_t;
struct cast2bool_visitor : public boost::static_visitor<bool> {
    Environment& m_env;
    cast2bool_visitor(Environment& env) : m_env(env) {}
    template <typename T>
    bool operator()(const T&) const
    {
        return false;
    }

    bool operator()(double d                 ) const { return bool(d); }
    bool operator()(int64_t d                ) const { return bool(d); }
    bool operator()(bool b                   ) const { return b;       }
    bool operator()(const string_t& s        ) const ;

    bool operator()(const varlisp::symbol& s ) const ;

    bool operator()(const List&              ) const
    {
        return false;
    }
};

}  // namespace varlisp

#endif /* __CAST2BOOL_VISITOR_HPP_1457680829__ */
