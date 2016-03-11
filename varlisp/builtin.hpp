#ifndef __BUILTIN_HPP_1457656469__
#define __BUILTIN_HPP_1457656469__

// 内建函数
// 首先，创建环境的时候，先做一个绑定
// env["+"] = builtin(TYPE_ADD);
// env["/"] = builtin(TYPE_DIV);
// ...
//
// 这样，就解耦了算术符号和具体实现代码——当然，更为奇葩的是，不用考虑优先级！

#include "object.hpp"

namespace varlisp {
    struct Builtin
    {
        enum type_t {
            TYPE_ADD, TYPE_SUB,
            TYPE_MUL, TYPE_DIV
        };
        struct para_length_t {
            int min;
            int max;
        };
        static const para_length_t  para_length[];

    public:
        explicit Builtin(type_t type);
        ~Builtin() = default;

    public:
        Builtin(Builtin&& ) = default;
        Builtin& operator = (Builtin&& ) = default;

    public:
        Builtin(const Builtin& ) = default;
        Builtin& operator = (const Builtin& ) = default;

    public:
        Object eval(varlisp::Environment& env, const varlisp::List& args);

    private:
        type_t m_type;
    };
} // namespace varlisp


#endif /* __BUILTIN_HPP_1457656469__ */
