#include "helpmsg_visitor.hpp"

#include "object.hpp"
#include "builtin_helper.hpp"
#include "environment.hpp"
#include "literal_hash.hpp"

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
    using namespace varlisp::detail::keywords_hash::literals;
    switch (varlisp::detail::keywords_hash::hash(s.m_data)) {
        case "if"_hash:
            return varlisp::string_t("keywords <if>\n"
                                     "\t(if condition consequent alternative)");
            break;

        case "else"_hash:
            return varlisp::string_t("keywords <else>\n"
                                     "\t see (help cond)");
            break;

        case "define"_hash:
            return varlisp::string_t("keywords <define>;\n"
                                     "\t(define var expr)\n"
                                     "\t(define (funcName args) \"help doc\" expr-list)");
            break;

        case "cond"_hash:
            return varlisp::string_t("keywords <cond>\n"
                                     "\t(cond (cond1 result1)... [(else expr])");
            // 另外一种写法：
            // (COND
            // 	(condition1   result1 )
            // 	(condition2   result2 )
            // 	...
            // 	(#t    resultN ) ) ; #t这里，是保证有值可以返回
            //! http://www.cis.upenn.edu/~matuszek/LispText/lisp-cond.html
            break;

        case "and"_hash:
            return varlisp::string_t("keywords <and>\n"
                                     "\t(and expr-list)");
            break;

        case "or"_hash:
            return varlisp::string_t("keywords <or>\n"
                                     "\t(or expr-list)");
            break;

        case "lambda"_hash:
            return varlisp::string_t("keywords <lambda>\n"
                                     "\t(lambda (arg...) \"help doc\" expr-list)");
            break;

        case "nil"_hash:
            return varlisp::string_t("keywords <nil>");
            break;

        case "list"_hash:
            return varlisp::string_t("keywords <list>\n\t(list ...)");
            break;

        default:
            // TODO keyword
            Object* it = m_env.find(s.m_data);
            if (!it) {
                SSS_POSITION_THROW(std::runtime_error, "symbol ", s.m_data,
                                   " not exsist");
            }
            Object obj;
            const Object& objref = varlisp::getAtomicValue(m_env, *it, obj);
            if (const varlisp::Builtin * p_tmp = boost::get<varlisp::Builtin>(&objref)) {
#if 0
                std::ostringstream oss;
                p_tmp->print(oss);
                return varlisp::string_t(std::move(oss.str()));
#else
                return get_helpdoc_mgr().get(s.m_data);
#endif
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
