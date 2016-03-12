#include "environment.hpp"

#include <sss/util/Memory.hpp>
#include <sss/log.hpp>

namespace varlisp {

    Environment::Environment(Environment * parent)
        : m_parent(parent)
    {
        // std::cout << __func__ << " " << this << " from " << parent << std::endl;
    }

    const Object * Environment::find(const std::string& name) const
    {
        const Environment * pe = this;
        const Object * ret = 0;
        do {
            auto it = pe->BaseT::find(name);
            if (it != pe->BaseT::cend()) {
                ret = &it->second;
            }
            pe = pe->m_parent;
        } while(pe && !ret);

        return ret;
    }

    Object* Environment::find(const std::string& name)
    {
        Environment * pe = this;
        Object * ret = 0;
        do {
            auto it = pe->BaseT::find(name);
            if (it != pe->BaseT::end()) {
                ret = &it->second;
            }
            pe = pe->m_parent;
        } while(pe && !ret);

        return ret;
    }

} // namespace varlisp
