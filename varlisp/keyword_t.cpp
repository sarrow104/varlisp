#include "keyword_t.hpp"
namespace varlisp {
bool keywords_t::is_keyword(const std::string& name)
{
    return keywords_t::help_msg(name)[0] != '\0';
}
const char * keywords_t::help_msg(const std::string& name)
{
    switch (varlisp::detail::keywords_hash::hash(name)) {
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

        default:
            return "";
    }
}
} // namespace varlisp
