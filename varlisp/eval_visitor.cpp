#include "eval_visitor.hpp"

#include "list.hpp"
#include "environment.hpp"

#include <sss/utlstring.hpp>

namespace varlisp {

    Object eval_visitor::operator() (const varlisp::symbol& s) const
    {
        Object * it = m_env.find(s.m_data);
        if (!it) {
            SSS_POSTION_THROW(std::runtime_error,
                              "symbol " << s.m_data << " not exsist");
        }
        return *it;
    }
    
} // namespace varlisp
