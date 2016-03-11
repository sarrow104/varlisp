#include "quote.hpp"

#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

namespace varlisp {
    void Quote::print(std::ostream& o) const
    {
        o << "(list";
        this->print_impl(o);
        o << ")";
    }

    void Quote::print_impl(std::ostream& o) const
    {
        const Quote * p = this;
        while (p && p->head.which()) {
            o << " ";
            boost::apply_visitor(print_visitor(o), p->head);
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
    }

    bool operator==(const Quote& lhs, const Quote& rhs)
    {
        return boost::apply_visitor(strict_equal_visitor(), lhs.head, rhs.head)
            && ((lhs.tail.empty() && rhs.tail.empty()) ||
                lhs.tail[0] == rhs.tail[0]);
    }
} // namespace varlisp
