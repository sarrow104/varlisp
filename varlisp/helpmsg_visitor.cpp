#include "helpmsg_visitor.hpp"

#include <sss/path.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "environment.hpp"
#include "keyword_t.hpp"

#include "detail/buitin_info_t.hpp"

namespace varlisp {

varlisp::string_t helpmsg_visitor::operator()(const varlisp::symbol& s) const
{
    const char * keyword_help_msg = varlisp::keywords_t::help_msg(s.m_data);
    if (keyword_help_msg[0]) {
        return varlisp::string_t(keyword_help_msg);
    }
    else {
        Object* it = m_env.find(s.m_data);
        if (!it) {
            SSS_POSITION_THROW(std::runtime_error, "symbol ", s.m_data,
                               " not exsist");
        }
        Object obj;
        const Object& objref = varlisp::getAtomicValue(m_env, *it, obj);
        if (const varlisp::Builtin * p_f = boost::get<varlisp::Builtin>(&objref)) {
            return varlisp::detail::get_builtin_infos()[p_f->type()].help_msg;
        }
        else if (const varlisp::Lambda * p_tmp = boost::get<varlisp::Lambda>(&objref)) {
            return p_tmp->help_doc;
        }
        else {
            return boost::apply_visitor(helpmsg_visitor(m_env), objref);
        }
    }
}
} // namespace varlisp
