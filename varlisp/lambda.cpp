#include <sss/util/PostionThrow.hpp>
#include <sss/log.hpp>
#include <sstream>

#include "lambda.hpp"

#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"
#include "eval_visitor.hpp"
#include "environment.hpp"

namespace varlisp {
    void Lambda::print(std::ostream& o) const
    {
        o << "(lambda (";
        std::copy(this->args.begin(), this->args.end(), std::ostream_iterator<std::string>(o, " "));
        o << ") ";
        boost::apply_visitor(print_visitor(o), this->body);
        o << ")";
    }

    Object Lambda::eval(Environment& env, const varlisp::List& true_args) const
    {
        SSS_LOG_EXPRESSION(sss::log::log_ERROR, true_args);
        SSS_LOG_EXPRESSION(sss::log::log_ERROR, *this);
        Environment inner(&env);
        if (this->args.size() != true_args.length()) {
            SSS_POSTION_THROW(std::runtime_error,
                              "expect " << this->args.size() << ", but given " << true_args.length());
        }
        const varlisp::List * p = &true_args;
        for (size_t i = 0;
             i != this->args.size();
             ++i, p = p->tail.empty() ? 0 : &p->tail[0])
        {
            if (!p) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "Not enough argument at " << i << "; name " << args[i]);
            }
            if (!p->head.which()) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "Empty argument at " << i << "; name " << args[i]);
            }

            SSS_LOG_EXPRESSION(sss::log::log_DEBUG, args[i]);
            inner[args[i]] = p->head;
        }
        return boost::apply_visitor(eval_visitor(inner), this->body);
    }

    // 不可比较
    bool operator==(const Lambda& lhs, const Lambda& rhs)
    {
        // return lhs.args == rhs.args
        //     && boost::apply_visitor(strict_equal_visitor(), lhs.body, rhs.body);
        return false;
    }

    bool operator<(const Lambda& lhs, const Lambda& rhs)
    {
        return false;
    }
} // namespace varlisp
