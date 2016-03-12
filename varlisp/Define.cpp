#include "Define.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

namespace varlisp {
    void Define::print(std::ostream& o) const
    {
        o << "(define " << this->name << " ";
        boost::apply_visitor(print_visitor(o), this->value);
        o << ")";
    }

    // 不可比较！
    bool operator==(const Define& lhs, const Define& rhs)
    {
//         return lhs.name == rhs.name &&
//             boost::apply_visitor(strict_equal_visitor(), lhs.value, rhs.value);
        return false;
    }

    // 不可比较！
    bool operator<(const Define& lhs, const Define& rhs)
    {
        return false;
    }

} // namespace varlisp
