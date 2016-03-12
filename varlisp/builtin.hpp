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

    struct Environment;

    struct Builtin
    {
        enum type_t {
            TYPE_ADD,
            TYPE_SUB,
            TYPE_MUL,
            TYPE_DIV,

            TYPE_POW,

            TYPE_EQ,

            TYPE_GT,
            TYPE_LT,

            TYPE_GE,
            TYPE_LE,

            TYPE_EVAL,
        };

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
        static void regist_builtin_function(Environment& env);

    public:
        void print(std::ostream& o) const;

        bool operator==(const Builtin& rhs) const
        {
            return this == &rhs || this->m_type == rhs.m_type;
        }

        bool operator<(const Builtin& rhs) const
        {
            return this != &rhs && this->m_type < rhs.m_type;
        }

    public:
        Object eval(varlisp::Environment& env, const varlisp::List& args) const;

    private:
        type_t m_type;
    };

    inline std::ostream& operator << (std::ostream& o, const Builtin& b)
    {
        b.print(o);
        return o;
    }
} // namespace varlisp


#endif /* __BUILTIN_HPP_1457656469__ */
