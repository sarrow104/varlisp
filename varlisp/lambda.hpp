#ifndef __LAMBDA_HPP_1457603378__
#define __LAMBDA_HPP_1457603378__

#include "object.hpp"

#include <vector>

namespace varlisp {
    struct Lambda
    {
        std::vector<std::string> args;  // 形式参数
        Object body;                    // 函数体

        Lambda() = default;
        Lambda(const std::vector<std::string>& a, const Object& b)
            : args(a), body(b)
        {
        }

        Lambda(std::vector<std::string>&& a, Object&& b)
            : args(std::move(a)), body(std::move(b))
        {
        }

        Lambda(const Lambda& ) = default;
        Lambda& operator=(const Lambda& ) = default;

        Lambda(Lambda&& ) = default;
        Lambda& operator=(Lambda&& ) = default;

        Object eval(Environment& env, const varlisp::List& args) const;

        void print(std::ostream& o) const;
    };

    inline std::ostream& operator << (std::ostream& o, const Lambda& l)
    {
        l.print(o);
        return o;
    }

    bool operator==(const Lambda& lhs, const Lambda& rhs);
    bool operator<(const Lambda& lhs, const Lambda& rhs);

} // namespace varlisp


#endif /* __LAMBDA_HPP_1457603378__ */
