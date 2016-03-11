#include "ifexpr.hpp"

#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

namespace varlisp {
    void IfExpr::print(std::ostream& o) const
    {
        o << "(if ";
        boost::apply_visitor(print_visitor(o), this->condition);
        o << " ";
        boost::apply_visitor(print_visitor(o), this->consequent);
        o << " ";
        boost::apply_visitor(print_visitor(o), this->alternative);
        o << ")";
    }

    bool operator==(const IfExpr& lhs, const IfExpr& rhs)
    {
        return
            boost::apply_visitor(strict_equal_visitor(), lhs.condition,     rhs.condition) &&
            boost::apply_visitor(strict_equal_visitor(), lhs.consequent,    rhs.consequent) &&
            boost::apply_visitor(strict_equal_visitor(), lhs.alternative,   rhs.alternative);
    }
} // namespace varlisp
