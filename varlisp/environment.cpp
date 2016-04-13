#include "environment.hpp"

#include <sss/util/Memory.hpp>
#include <sss/log.hpp>

namespace varlisp {

    Environment::Environment(Environment * parent)
        : m_parent(parent), m_interpreter(0)
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

    Interpreter * Environment::getInterpreter() const
    {
        const Environment * pe = this;
        Interpreter * ret = 0;
        do {
            if (pe->m_parent == 0) {
                return pe->m_interpreter;
            }
            pe = pe->m_parent;
        } while (pe);
        return ret;
    }

    Interpreter * Environment::setInterpreter(Interpreter& interpreter)
    {
        this->m_interpreter = &interpreter;
    }
} // namespace varlisp
