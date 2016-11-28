#include "helpmsg_visitor.hpp"

#include "object.hpp"
#include "builtin_helper.hpp"
#include "environment.hpp"

#include "keyword_t.hpp"

#include "builtin_helpdoc_mgr.hpp"

#include <sss/path.hpp>

namespace varlisp {

std::string get_helpdoc_cfg_fname()
{
    std::string doc_path = sss::path::dirname(sss::path::getbin());
    sss::path::append(doc_path, "helpdoc/builtin.txt");
    return doc_path;
}

varlisp::builtin_helpdoc_mgr& get_helpdoc_mgr()
{
    static varlisp::builtin_helpdoc_mgr mgr{get_helpdoc_cfg_fname()};
    return mgr;
}

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
        if (boost::get<varlisp::Builtin>(&objref)) {
            return get_helpdoc_mgr().get(s.m_data);
        }
        else if (const auto * p_tmp = boost::get<varlisp::Lambda>(&objref)) {
            return p_tmp->help_doc;
        }
        else {
            return boost::apply_visitor(helpmsg_visitor(m_env), objref);
        }
    }
}
} // namespace varlisp
