#include "keyword_t.hpp"

#include <stdexcept>

#include <sss/util/PostionThrow.hpp>
#include <sss/string_view.hpp>
#include <sss/algorithm.hpp>

#include "String.hpp"

namespace varlisp {

namespace detail {

int index_into_keyword_info(keywords_t::kw_type_t t)
{
    switch (t) {
        case keywords_t::kw_IF:
            return 0;
        case keywords_t::kw_ELSE:
            return 1;
        case keywords_t::kw_DEFINE:
            return 2;
        case keywords_t::kw_COND:
            return 3;
        case keywords_t::kw_AND:
            return 4;
        case keywords_t::kw_OR:
            return 5;
        case keywords_t::kw_LAMBDA:
            return 6;
        case keywords_t::kw_NIL:
            return 7;
        case keywords_t::kw_LIST:
            return 8;
        case keywords_t::kw_QUOTE:
            return 9;
        case keywords_t::kw_CONTEXT:
            return 10;

        case keywords_t::kw_NONE:
            // fallthrough
        default:
            return -1;
    }
}

} // namespace detail

struct keywords_info_t {
    sss::string_view name;
    sss::string_view help_msg;
};

const keywords_info_t keywords_info[] = {
    {"if"     , "keywords <if>\n\t(if condition consequent alternative); only #t will be treat as true, otherwise will be false"},
    {"else"   , "keywords <else>\n\t see (help cond)"},
    {"define" , "keywords <define>\n\t(define var expr)\n\t(define (funcName args) \"help doc\" expr-list)"},
    {"cond"   , "keywords <cond>\n\t(cond (cond1 result1)... [(else expr)])\n\t NOTE only #t will be treat as true then run this branch"},
    // 另外一种写法：
    // (COND
    // 	(condition1   result1 )
    // 	(condition2   result2 )
    // 	...
    // 	(#t    resultN ) ) ; #t这里，是保证有值可以返回
    //! http://www.cis.upenn.edu/~matuszek/LispText/lisp-cond.html
    {"and"     , "keywords <and>\n\t(and expr-list)" },
    {"or"      , "keywords <or>\n\t(or expr-list)"},
    {"lambda"  , "keywords <lambda>\n\t(lambda (arg...) \"help doc\" expr-list)"},
    {"nil"     , "keywords <nil>"},
    {"list"    , "keywords <list>\n\t(list ...)"},
    {"quote"   , "kwyeords <quote>\n\t(quote ...)\n\t'..."},
    {"context" , "kwyeords <quote>\n\t(context ...)\n\t{...}"},
};

bool keywords_t::is_keyword(const std::string& name)
{
    int index = detail::index_into_keyword_info(keywords_t::gen_hash(name));
    return index >= 0 && size_t(index) < sss::size(keywords_info);
}

keywords_t::kw_type_t keywords_t::gen_hash(const std::string& name)
{
    return (keywords_t::kw_type_t)varlisp::detail::keywords_hash::hash(name);
}

const std::vector<sss::string_view>& keywords_t::get_keywords_vector()
{
    static std::vector<sss::string_view> l_keywords_vector;
    static bool init_kw = [&](std::vector<sss::string_view>& v)->bool {
        for (auto& item : keywords_info ) {
            l_keywords_vector.push_back(item.name);
        }
        return true;
    }(l_keywords_vector);
    return l_keywords_vector;
}

sss::string_view keywords_t::name() const
{
    int index = detail::index_into_keyword_info(m_hash_value);
    if (index < 0) {
        SSS_POSITION_THROW(std::runtime_error, m_hash_value);
    }
    return keywords_info[index].name;
}

sss::string_view keywords_t::help_msg() const {
    int index = detail::index_into_keyword_info(m_hash_value);
    if (index < 0) {
        SSS_POSITION_THROW(std::runtime_error, m_hash_value);
    }
    return keywords_info[index].help_msg;
}

void keywords_t::print(std::ostream& o) const
{
    o << this->name();
}

} // namespace varlisp
