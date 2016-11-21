#include "cast2bool_visitor.hpp"
#include "object.hpp"
#include "environment.hpp"
#include "String.hpp"

#include "builtin_helper.hpp"
namespace varlisp {

bool cast2bool_visitor::operator()(const string_t& s) const
{
    return (s == sss::string_view("#t") || s == sss::string_view("true"));
}

bool cast2bool_visitor::operator()(const varlisp::symbol& s) const
{
    Object* it = m_env.find(s.m_data);
    if (!it) {
        SSS_POSITION_THROW(std::runtime_error, "symbol ", s.m_data,
                          " not exists!");
    }
    return varlisp::is_true(m_env, *it);
}

} // namespace varlisp
