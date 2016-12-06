#pragma once

#include <iosfwd>

#include <sss/string_view.hpp>

#include "literal_hash.hpp"

namespace varlisp {

struct String;
typedef String string_t;
/**
 * @brief 
 *    标识关键字；提供关键字的一些说明和识别功能；
 */
struct keywords_t
{
    enum kw_type_t {
        // using namespace varlisp::detail::keywords_hash;
        kw_NONE    = 0,
        kw_IF      = varlisp::detail::keywords_hash::hash("if"),
        kw_ELSE    = varlisp::detail::keywords_hash::hash("else"),
        kw_DEFINE  = varlisp::detail::keywords_hash::hash("define"),
        kw_COND    = varlisp::detail::keywords_hash::hash("cond"),
        kw_AND     = varlisp::detail::keywords_hash::hash("and"),
        kw_OR      = varlisp::detail::keywords_hash::hash("or"),
        kw_LAMBDA  = varlisp::detail::keywords_hash::hash("lambda"),
        kw_NIL     = varlisp::detail::keywords_hash::hash("nil"),
        kw_LIST    = varlisp::detail::keywords_hash::hash("list"),  // list 和 quote 等效
        kw_QUOTE   = varlisp::detail::keywords_hash::hash("quote"),
        kw_CONTEXT = varlisp::detail::keywords_hash::hash("context"),
    };
    static bool is_keyword(const std::string& name);
    static const char * help_msg(const std::string& name);

    static kw_type_t gen_hash(const std::string& name);
    static const string_t& gen_name(kw_type_t t);
    
    keywords_t(kw_type_t t)
        : m_hash_value(t)
    {
    }
    explicit keywords_t(const char * name)
    {
        succeed_or_throw(name);
    }
    template<size_t N>
    explicit keywords_t(const char (&name)[N])
    {
        succeed_or_throw(name);
    }
    ~keywords_t() = default;

    explicit keywords_t(const std::string& name)
    {
        succeed_or_throw(name);
    }

    void succeed_or_throw(const std::string& name)
    {
        if (!keywords_t::is_keyword(name)) {
            throw name;
        }
        m_hash_value = (kw_type_t)varlisp::detail::keywords_hash::hash(name);
    }

    bool operator ==(const keywords_t& rhs) const {
        return m_hash_value == rhs.m_hash_value;
    }

    bool operator <(const keywords_t& rhs) const {
        return m_hash_value < rhs.m_hash_value;
    }

    sss::string_view name() const;

    void print(std::ostream& o) const;

    kw_type_t type() const {
        return m_hash_value;
    }

private:
    kw_type_t m_hash_value;
};

inline std::ostream& operator << (std::ostream& o, const keywords_t& key)
{
    key.print(o);
    return o;
}

} // namespace varlisp
