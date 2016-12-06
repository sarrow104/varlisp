#include "helpmsg_visitor.hpp"

#include <sss/path.hpp>
#include <sss/colorlog.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "environment.hpp"
#include "keyword_t.hpp"

#include "detail/buitin_info_t.hpp"

namespace varlisp {

varlisp::string_t helpmsg_visitor::operator()(const varlisp::symbol& s) const
{
    varlisp::string_t help_msg;
    Object* it = m_env.find(s.m_data);
    if (!it) {
        SSS_POSITION_THROW(std::runtime_error, "symbol ", s.m_data,
                           " not exsist");
    }
    Object obj;
    const Object& objref = varlisp::getAtomicValue(m_env, *it, obj);
    if (const varlisp::Builtin * p_f = boost::get<varlisp::Builtin>(&objref)) {
        help_msg = varlisp::detail::get_builtin_infos()[p_f->type()].help_msg;
    }
    else if (const varlisp::Lambda * p_tmp = boost::get<varlisp::Lambda>(&objref)) {
        help_msg = p_tmp->help_doc;
    }
    else {
        help_msg = boost::apply_visitor(helpmsg_visitor(m_env), objref);
    }
    if (help_msg.empty()) {
        COLOG_ERROR("there is no help-msg for `", s, "`");
    }
    return help_msg;
}

varlisp::string_t helpmsg_visitor::operator()(const varlisp::keywords_t& k ) const
{
    return varlisp::string_t(k.help_msg(), true);
}

} // namespace varlisp
