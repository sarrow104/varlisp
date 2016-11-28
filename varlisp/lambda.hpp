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
    std::vector<std::string> args;  // 形式参数
    std::vector<Object> body;       // 函数体
    varlisp::string_t   help_doc;   // 帮助信息

    Lambda() = default;
    Lambda(const std::vector<std::string>& a, varlisp::string_t msg, const std::vector<Object>& b)
        : args(a), body(b), help_doc(msg)
    {
    }

    Lambda(std::vector<std::string>&& a, varlisp::string_t&& m, std::vector<Object>&& b)
        : args(std::move(a)), body(std::move(b)), help_doc(std::move(m))
    {
    }

    Lambda(const Lambda&) = default;
    Lambda& operator=(const Lambda&) = default;

    Lambda(Lambda&&) = default;
    Lambda& operator=(Lambda&&) = default;

    Object eval(Environment& env, const varlisp::List& args) const;

    void print(std::ostream& o) const;
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
