#include "is_instant_visitor.hpp"

#include "list.hpp"

namespace varlisp {
bool is_instant_visitor::operator()(const varlisp::List& v) const { return v.is_squote(); }
} // namespace varlisp
