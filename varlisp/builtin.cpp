#include "builtin.hpp"

#include <sss/util/PostionThrow.hpp>

namespace varlisp {
    // 参数格式；
    // 闭区间；-1表示无穷
    const Builtin::para_length_t Builtin::para_length[]
        = {
            {1, -1},
            {1, -1},
            {2, -1},
            {2, -1},
        };

    Object eval_add(varlisp::Environment& env, const varlisp::List& args)
    {
    }
    Object eval_sub(varlisp::Environment& env, const varlisp::List& args)
    {
    }
    Object eval_mul(varlisp::Environment& env, const varlisp::List& args)
    {
    }
    Object eval_div(varlisp::Environment& env, const varlisp::List& args)
    {
    }

    typedef Object (*eval_t)(varlisp::Environment& env, const varlisp::List& args);
    const eval_t         eval_table[]
        = {
            &eval_add,
            &eval_sub,
            &eval_mul,
            &eval_div
        };

    Builtin::Builtin(type_t type)
        : m_type(type)
    {
    }

    Object Builtin::eval(varlisp::Environment& env, const varlisp::List& args)
    {
        int arg_length = args.length();
        int arg_min = Builtin::para_length[m_type].min;
        int arg_max = Builtin::para_length[m_type].max;
        if (arg_min > 0 && arg_length < arg_min) {
            SSS_POSTION_THROW(std::runtime_error,
                              " need at least " << arg_min << " parameters. "
                              << " but provided " << arg_length);
        }
        if (arg_max > 0 && arg_length > arg_max) {
            SSS_POSTION_THROW(std::runtime_error,
                              " need at most " << arg_max << " parameters."
                              << " but provided " << arg_length);
        }
        return eval_table[m_type](env, args);
    }
} // namespace varlisp
