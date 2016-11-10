#include "object.hpp"

#include "eval_visitor.hpp"

namespace varlisp {
struct List;
struct Environment;

template <typename T>
inline const T* getTypedValue(varlisp::Environment& env, const varlisp::Object& value,
                       Object& obj)
{
    obj = boost::apply_visitor(eval_visitor(env), value);
    const T* p_value = boost::get<const T>(&obj);
    return p_value;
}

inline bool is_true(varlisp::Environment& env, const varlisp::Object& obj)
{
    Object res;
    const bool * p_bool = getTypedValue<bool>(env, obj, res);
    return p_bool && *p_bool;
}

const varlisp::List* getFirstListPtrFromArg(varlisp::Environment& env,
                                            const varlisp::List& args,
                                            Object& tmp);
}  // namespace varlisp
