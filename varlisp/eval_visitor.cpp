#include "eval_visitor.hpp"

#include <sss/utlstring.hpp>
#include <sss/colorlog.hpp>

#include "environment.hpp"
#include "list.hpp"

namespace varlisp {

Object eval_visitor::operator()(const varlisp::symbol& s) const
{
    Object* it = m_env.deep_find(s.name());

    if (!it) {
        SSS_POSITION_THROW(std::runtime_error, "symbol ", s.name(),
                          " not exsist");
    }
    COLOG_DEBUG(s, *it);
    if (auto * p_list = boost::get<varlisp::List>(&*it)) {
        COLOG_DEBUG(p_list->is_quoted());
    }
    return *it;
}

}  // namespace varlisp
