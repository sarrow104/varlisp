#include "lambda.hpp"

#include <sstream>

#include <sss/log.hpp>
#include <sss/util/PostionThrow.hpp>

#include "environment.hpp"
#include "eval_visitor.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

namespace varlisp {
void Lambda::print(std::ostream& o) const
{
    o << "(lambda (";
    if (!this->m_args.empty()) {
        std::copy(this->m_args.begin(), this->m_args.end() - 1,
                  std::ostream_iterator<std::string>(o, " "));
        o << this->m_args.back();
    }
    o << ") ";
    if (!this->m_help_doc.empty()) {
        o << sss::raw_string(this->m_help_doc) << " ";
    }
    if (!this->m_body.empty()) {
        bool is_first = true;
        for (const auto& obj : this->m_body) {
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

varlisp::string_t Lambda::gen_help_msg(const std::string& name) const
{
    std::ostringstream oss;
    oss << "(" << name << " ";
    if (!this->m_args.empty()) {
        std::copy(this->m_args.begin(), this->m_args.end() - 1,
                  std::ostream_iterator<std::string>(oss, " "));
        oss << this->m_args.back();
    }
    oss << ")";
    return string_t(std::move(oss.str()));
}

Object Lambda::eval(Environment& env, const varlisp::List& true_args) const
{
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, true_args);
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, *this);
    Environment inner(&env);
    if (this->m_args.size() < true_args.length()) {
        SSS_POSITION_THROW(std::runtime_error, *this, " expect ",
                           this->m_args.size(), " argument, but given ",
                           true_args.length(), " argument: ", true_args);
    }
    auto p = true_args.begin();
    for (size_t i = 0, isize = std::min<size_t>(this->m_args.size(), true_args.length());
         i != isize;
         ++i, ++p)
    {
        assert(p != true_args.end());
        if (!(*p).which()) {
            SSS_POSITION_THROW(std::runtime_error, "Empty argument at ", i,
                              "; name ", m_args[i]);
        }

        // SSS_LOG_EXPRESSION(sss::log::log_DEBUG, m_args[i]);
        inner[m_args[i]] = boost::apply_visitor(eval_visitor(env), *p);
    }
    // NOTE 2021-01-26
    // padding nil while not enough parameters
    for (size_t i = true_args.length(); i < this->m_args.size(); ++i)
    {
        inner[m_args[i]] = Nill{};
    }

    size_t i = 0;
    Object rst;
    for (const auto& obj : this->m_body) {
        if (i == this->m_body.size() - 1) {
            rst = boost::apply_visitor(eval_visitor(inner), obj);
        }
        else {
            boost::apply_visitor(eval_visitor(inner), obj);
        }
        ++i;
    }
    return rst;
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
