#ifndef __QUOTE_HPP_1457603131__
#define __QUOTE_HPP_1457603131__

#include "object.hpp"

#include <vector>

namespace varlisp {
    // Quote eval 之后，等于一个List
    struct Quote
    {
        Quote() {}
        Quote(const Object& h, const Quote& t)
            : head(h)
        {
            tail.push_back(t);
        }
        explicit Quote(const Object& o)
            : head(o)
        {
        }
        // bool equal(const Quote& rhs) const;
        Object head;
        std::vector<Quote> tail;
        void print(std::ostream& o) const;
        void print_impl(std::ostream& o) const;
        bool is_empty() const {
            return this->head.which() == 0 && this->tail.empty();
        }
        int length() const {
            int len = 0;
            const Quote * p = this;
            while (p && p->head.which()) {
                len++;
                if (p->tail.empty()) {
                    p = 0;
                }
                else {
                    p = &p->tail[0];
                }
            }
            return len;
        }
    };

    inline std::ostream& operator << (std::ostream& o, const Quote& d)
    {
        d.print(o);
        return o;
    }

    bool operator==(const Quote& lhs, const Quote& rhs);
} // namespace varlisp


#endif /* __QUOTE_HPP_1457603131__ */
