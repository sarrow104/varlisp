#pragma once

#include "literal_hash.hpp"

namespace varlisp {
/**
 * @brief 
 *    标识关键字；提供关键字的一些说明和识别功能；
 */
struct keywords_t
{
    enum kw_type_t {
        // using namespace varlisp::detail::keywords_hash;
        kw_NONE   = 0,
        kw_IF     = varlisp::detail::keywords_hash::hash("if"),
        kw_ELSE   = varlisp::detail::keywords_hash::hash("else"),
        kw_DEFINE = varlisp::detail::keywords_hash::hash("define"),
        kw_COND   = varlisp::detail::keywords_hash::hash("cond"),
        kw_AND    = varlisp::detail::keywords_hash::hash("and"),
        kw_OR     = varlisp::detail::keywords_hash::hash("or"),
        kw_LAMBDA = varlisp::detail::keywords_hash::hash("lambda"),
        kw_NIL    = varlisp::detail::keywords_hash::hash("nil"),
        kw_LIST   = varlisp::detail::keywords_hash::hash("list")
    };
    static bool is_keyword(const std::string& name);
    static const char * help_msg(const std::string& name);
};

} // namespace varlisp
