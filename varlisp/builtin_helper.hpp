#include "object.hpp"

namespace varlisp {
struct List;
struct Environment;

bool is_literal_list(const varlisp::List& l);
const varlisp::List* getFirstListPtrFromArg(varlisp::Environment& env,
                                            const varlisp::List& args,
                                            Object& tmp);
}  // namespace varlisp
