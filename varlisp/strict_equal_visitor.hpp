#ifndef __STRICT_EQUAL_VISITOR_HPP_1457603635__
#define __STRICT_EQUAL_VISITOR_HPP_1457603635__

#include <boost/variant.hpp>

namespace varlisp {
    struct strict_equal_visitor : boost::static_visitor<bool>
    {
        template <typename T, typename U>
            bool operator() (const T&, const U&) const
            {
                return false;
            }

        template <typename T>
            bool operator() (const T& lhs, const T& rhs) const
            {
                return lhs == rhs;
            }
    };
} // namespace varlisp


#endif /* __STRICT_EQUAL_VISITOR_HPP_1457603635__ */
