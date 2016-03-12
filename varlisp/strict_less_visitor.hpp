#ifndef __STRICT_LESS_VISITOR_HPP_1457699825__
#define __STRICT_LESS_VISITOR_HPP_1457699825__

#include <boost/variant.hpp>

namespace varlisp {
    struct strict_less_visitor : boost::static_visitor<bool>
    {
        template <typename T, typename U>
            bool operator() (const T&, const U&) const
            {
                return false;
            }

        template <typename T>
            bool operator() (const T& lhs, const T& rhs) const
            {
                return lhs < rhs;
            }
    };
} // namespace varlisp



#endif /* __STRICT_LESS_VISITOR_HPP_1457699825__ */

