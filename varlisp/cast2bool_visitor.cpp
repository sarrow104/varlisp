#include "cast2bool_visitor.hpp"
#include "object.hpp"
#include "environment.hpp"

#include "builtin_helper.hpp"
namespace varlisp {

bool cast2bool_visitor::operator()(const std::string& s) const
{
    return (s == "#t" || s == "true");
}

bool cast2bool_visitor::operator()(const varlisp::symbol& s) const
{
    Object* it = m_env.find(s.m_data);
    if (!it) {
        SSS_POSTION_THROW(std::runtime_error, "symbol ", s.m_data,
                          " not exists!");
    }
    return varlisp::is_true(m_env, *it);
}

} // namespace varlisp
