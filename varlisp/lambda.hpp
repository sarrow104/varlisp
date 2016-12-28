#ifndef __LAMBDA_HPP_1457603378__
#define __LAMBDA_HPP_1457603378__

#include <vector>

#include "object.hpp"

namespace varlisp {
// Lambda
// 允许空参数列表——直接()即可，不用'()或者(list)
// 函数体相当于表达式的数组；不允许为空；至少是一个'();
// 相当于必须有函数体；
//
// 另外Lisp语法，可以提供一个&rest形参，以表示不定参数；具名以外，提供的参
// 数，都会绑定给它，作为列表？
// (define (arg-list) ["help-doc"] (body-expr-list))
struct Lambda {
private:
    std::vector<std::string>    m_args;       // 形式参数
    std::vector<Object>         m_body;       // 函数体
    varlisp::string_t           m_help_doc;   // 帮助信息
    varlisp::Environment *      m_penv;       // 方法所属环境
    // NOTE 如果要实现闭包的话，那么闭包所引用到的变量，以及其定义，应该如何序列
    // 化到外部文件？

public:
    Lambda() = default;
    Lambda(const std::vector<std::string>& a, varlisp::string_t msg, const std::vector<Object>& b)
        : m_args(a), m_body(b), m_help_doc(msg)
    {
    }

    Lambda(std::vector<std::string>&& a, varlisp::string_t&& m, std::vector<Object>&& b)
        : m_args(std::move(a)), m_body(std::move(b)), m_help_doc(std::move(m))
    {
    }

    Lambda(const Lambda&) = default;
    Lambda& operator=(const Lambda&) = default;

    Lambda(Lambda&&) = default;
    Lambda& operator=(Lambda&&) = default;

    Object eval(Environment& env, const varlisp::List& args) const;

    void print(std::ostream& o) const;
    int  argument_count() const
    {
        return m_args.size();
    }
    varlisp::string_t help_msg() const
    {
        return this->m_help_doc;
    }

    varlisp::string_t gen_help_msg(const std::string& name) const;
};

inline std::ostream& operator<<(std::ostream& o, const Lambda& l)
{
    l.print(o);
    return o;
}

bool operator==(const Lambda& lhs, const Lambda& rhs);
bool operator<(const Lambda& lhs, const Lambda& rhs);

}  // namespace varlisp

#endif /* __LAMBDA_HPP_1457603378__ */
