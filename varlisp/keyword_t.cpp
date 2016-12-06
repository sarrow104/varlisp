#include "keyword_t.hpp"

#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "String.hpp"

namespace varlisp {
bool keywords_t::is_keyword(const std::string& name)
{
    return keywords_t::help_msg(name)[0] != '\0';
}
const char * keywords_t::help_msg(const std::string& name)
{
    switch (keywords_t::gen_hash(name)) {
        case kw_NONE:
            return "";
            break;

        case kw_IF:
            return "keywords <if>\n"
                "\t(if condition consequent alternative)";
            break;

        case kw_ELSE:
            return "keywords <else>\n"
                "\t see (help cond)";
            break;

        case kw_DEFINE:
            return "keywords <define>;\n"
                "\t(define var expr)\n"
                "\t(define (funcName args) \"help doc\" expr-list)";
            break;

        case kw_COND:
            return "keywords <cond>\n"
                "\t(cond (cond1 result1)... [(else expr])";
            // 另外一种写法：
            // (COND
            // 	(condition1   result1 )
            // 	(condition2   result2 )
            // 	...
            // 	(#t    resultN ) ) ; #t这里，是保证有值可以返回
            //! http://www.cis.upenn.edu/~matuszek/LispText/lisp-cond.html
            break;

        case kw_AND:
            return "keywords <and>\n"
                "\t(and expr-list)";
            break;

        case kw_OR:
            return "keywords <or>\n"
                "\t(or expr-list)";
            break;

        case kw_LAMBDA:
            return "keywords <lambda>\n"
                "\t(lambda (arg...) \"help doc\" expr-list)";
            break;

        case kw_NIL:
            return "keywords <nil>";
            break;

        case kw_LIST:
            return "keywords <list>\n\t(list ...)";
            break;

        case kw_QUOTE:
            return "kwyeords <quote>\n\t(quote ...)\n\t'...";

        case kw_CONTEXT:
            return "kwyeords <quote>\n\t(context ...)\n\t{...}";

        default:
            SSS_POSITION_THROW(std::runtime_error,
                               name);
            return "";
    }
}

keywords_t::kw_type_t keywords_t::gen_hash(const std::string& name)
{
    uint64_t hash =
        varlisp::detail::keywords_hash::hash(name);
    switch (hash) {
        case kw_IF:
            return kw_IF;

        case kw_ELSE:
            return kw_ELSE;

        case kw_DEFINE:
            return kw_DEFINE;

        case kw_COND:
            return kw_COND;
            // 另外一种写法：
            // (COND
            // 	(condition1   result1 )
            // 	(condition2   result2 )
            // 	...
            // 	(#t    resultN ) ) ; #t这里，是保证有值可以返回
            //! http://www.cis.upenn.edu/~matuszek/LispText/lisp-cond.html

        case kw_AND:
            return kw_AND;

        case kw_OR:
            return kw_OR;

        case kw_LAMBDA:
            return kw_LAMBDA;

        case kw_NIL:
            return kw_NIL;

        case kw_LIST:
            return kw_LIST;

        case kw_QUOTE:
            return kw_QUOTE;

        case kw_CONTEXT:
            return kw_CONTEXT;

        default:
            return kw_NONE;
    }
}

const string_t& keywords_t::gen_name(keywords_t::kw_type_t t)
{
    static string_t name_none{""};
#ifndef MAKE_NAME
#define MAKE_NAME(a) static string_t name_##a{#a};
#endif

    MAKE_NAME(if);
    MAKE_NAME(else);
    MAKE_NAME(define);
    MAKE_NAME(cond);
    MAKE_NAME(and);
    MAKE_NAME(or);
    MAKE_NAME(lambda);
    MAKE_NAME(nil);
    MAKE_NAME(list);
    MAKE_NAME(quote);
    MAKE_NAME(context);

#undef MAKE_NAME

    switch (t) {
        case kw_NONE:
            return name_none;

        case kw_IF:
            return name_if;

        case kw_ELSE:
            return name_else;

        case kw_DEFINE:
            return name_define;

        case kw_COND:
            return name_cond;

        case kw_AND:
            return name_and;

        case kw_OR:
            return name_or;

        case kw_LAMBDA:
            return name_lambda;

        case kw_NIL:
            return name_nil;

        case kw_LIST:
            return name_list;

        case kw_QUOTE:
            return name_quote;

        case kw_CONTEXT:
            return name_context;

        default:
            SSS_POSITION_THROW(std::runtime_error,
                               t);
    }
}

sss::string_view keywords_t::name() const {
    return keywords_t::gen_name(m_hash_value).to_string_view();
}

void keywords_t::print(std::ostream& o) const
{
    o << keywords_t::gen_name(this->m_hash_value);
}

} // namespace varlisp
