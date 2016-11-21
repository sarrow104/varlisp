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
    bool need_exist = false;
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

    if (need_exist) {
        exit(1);
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

// 不可比较
bool operator==(const Lambda& lhs, const Lambda& rhs)
{
    // return lhs.args == rhs.args
    //     && boost::apply_visitor(strict_equal_visitor(), lhs.body, rhs.body);
    return false;
}

bool operator<(const Lambda& lhs, const Lambda& rhs) { return false; }
}  // namespace varlisp
