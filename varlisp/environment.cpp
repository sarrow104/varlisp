#include "environment.hpp"

#include <sss/util/Memory.hpp>

namespace varlisp {

    Environment::Environment(Environment * parent)
        : m_parent(parent)
    {
        std::cout << __func__ << " from " << parent << std::endl;
    }

    Environment::const_iterator Environment::find(const std::string& name) const
    {
        const_iterator it = this->BaseT::find(name);
        if (it != this->BaseT::cend()) {
            return it;
        }
        else {
            const Environment * pe = this;
            while (pe->m_parent) {
                it = pe->m_parent->BaseT::find(name);
                if (it != pe->BaseT::cend()) {
                    break;
                }
                pe = pe->m_parent;
            }
        }
        return it;
    }

    Environment::iterator Environment::find(const std::string& name)
    {
        iterator it = this->BaseT::find(name);
        if (it != this->BaseT::end()) {
            return it;
        }
        else {
            const Environment * pe = this;
            while (pe->m_parent) {
                it = pe->m_parent->BaseT::find(name);
                if (it != pe->BaseT::end()) {
                    break;
                }
                pe = pe->m_parent;
            }
        }
        return it;
    }

} // namespace varlisp
