#include "is_symbol.hpp"

#include "../keyword_t.hpp"

namespace varlisp {
namespace detail {

bool is_symbol(const std::string& name)
{
    varlisp::Tokenizer t{name};
    auto tok = t.top();
    if (t.lookahead_nothrow(1).which()) {
        return false;
    }
    varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&tok);
    if (!p_sym) {
        return false;
    }
    if (p_sym->name() != name) {
        return false;
    }
    if (varlisp::keywords_t::is_keyword(p_sym->name())) {
        return false;
    }
    return true;
}
} // namespace detail
} // namespace varlisp

