#include "object.hpp"

namespace varlisp {
struct List;
struct Environment;

const varlisp::List* getFirstListPtrFromArg(varlisp::Environment& env,
                                            const varlisp::List& args,
                                            Object& tmp);
}  // namespace varlisp
