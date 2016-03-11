#ifndef __PRINT_VISITOR_HPP_1457603502__
#define __PRINT_VISITOR_HPP_1457603502__

#include <boost/variant.hpp>

#include "object.hpp"
#include "util.hpp"

namespace varlisp {

    struct print_visitor : public boost::static_visitor<void>
    {
        std::ostream& m_o;
        print_visitor(std::ostream& o)
            : m_o(o)
        {}

        template<typename T>
            void operator() (const T& v) const
            {
                m_o << v;
            }

        void operator() (const Empty& ) const
        {
        }

        void operator() (bool v) const
        {
            m_o << (v ? "#t" : "#f");
        }

        void operator() (const std::string& v) const
        {
            m_o << util::escape(v);
        }

        void operator() (const varlisp::symbol& s) const
        {
            m_o << s.m_data;
        }
    };

} // namespace varlisp


#endif /* __PRINT_VISITOR_HPP_1457603502__ */
