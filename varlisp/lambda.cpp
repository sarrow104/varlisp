#include "lambda.hpp"

#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

namespace varlisp {
    void Lambda::print(std::ostream& o) const
    {
        o << "(Lambda (";
        std::copy(this->args.begin(), this->args.end(), std::ostream_iterator<std::string>(o, " "));
        o << ") ";
        boost::apply_visitor(print_visitor(o), this->body);
        o << ")";
    }

    bool operator==(const Lambda& lhs, const Lambda& rhs)
    {
        return lhs.args == rhs.args
            && boost::apply_visitor(strict_equal_visitor(), lhs.body, rhs.body);
    }
} // namespace varlisp
