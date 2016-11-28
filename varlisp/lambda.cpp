#include <sss/log.hpp>
#include <sss/util/PostionThrow.hpp>
#include <sstream>

#include "lambda.hpp"

#include "environment.hpp"
#include "eval_visitor.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

namespace varlisp {
void Lambda::print(std::ostream& o) const
{
    o << "(lambda (";
    if (!this->args.empty()) {
        std::copy(this->args.begin(), this->args.end() - 1,
                  std::ostream_iterator<std::string>(o, " "));
        o << this->args.back();
    }
    o << ") ";
    if (!this->body.empty()) {
        bool is_first = true;
        for (const auto& obj : this->body) {
            if (is_first) {
                is_first = false;
            }
            else {
                o << " ";
            }
            boost::apply_visitor(print_visitor(o), obj);
        }
    }
    o << ")";
}

Object Lambda::eval(Environment& env, const varlisp::List& true_args) const
{
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, true_args);
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, *this);
    Environment inner(&env);
    if (this->args.size() != true_args.length()) {
        std::cout << *this << std::endl;
        std::cout << true_args << std::endl;
        SSS_POSITION_THROW(std::runtime_error, "expect ", this->args.size(),
                          ", but given ", true_args.length());
    }
    const varlisp::List* p = &true_args;
    for (size_t i = 0; i != this->args.size();
         ++i, p = p->tail.empty() ? 0 : &p->tail[0]) {
        if (!p) {
            SSS_POSITION_THROW(std::runtime_error, "Not enough argument at ", i,
                              "; name ", args[i]);
        }
        if (!p->head.which()) {
            SSS_POSITION_THROW(std::runtime_error, "Empty argument at ", i,
                              "; name ", args[i]);
        }

        // SSS_LOG_EXPRESSION(sss::log::log_DEBUG, args[i]);
        inner[args[i]] = boost::apply_visitor(eval_visitor(env), p->head);
    }

    size_t i = 0;
    for (const auto& obj : this->body) {
        if (i == this->body.size() - 1) {
            return boost::apply_visitor(eval_visitor(inner), obj);
        }
        else {
            boost::apply_visitor(eval_visitor(inner), obj);
        }
        ++i;
    }
    // return boost::apply_visitor(eval_visitor(inner), this->body);
}

// 比较通过递归完成；分别比较各个部分元素
// 但实际使用的时候，比如 operator < 的实现，需要先后判断 == 和 <
// 导致两次循环比较。
// 效率太低。最好，先实现一个compare函数，获得一个-1,0,1三值。
// 再比较。
// 所以，最好的办法，是序列化函数之后，比较字符串！
bool operator==(const Lambda& lhs, const Lambda& rhs)
{
    std::ostringstream os1;
    std::ostringstream os2;
    lhs.print(os1);
    rhs.print(os2);
    return os1.str() == os2.str();
}

bool operator<(const Lambda& lhs, const Lambda& rhs) {
    std::ostringstream os1;
    std::ostringstream os2;
    lhs.print(os1);
    rhs.print(os2);
    return os1.str() < os2.str();
}
}  // namespace varlisp
