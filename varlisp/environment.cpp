#include "environment.hpp"

#include <sss/log.hpp>
#include <sss/util/Memory.hpp>
#include <sss/colorlog.hpp>

#include "eval_visitor.hpp"

namespace varlisp {

Environment::Environment(Environment* parent)
    : m_parent(parent), m_interpreter(0)
{
    COLOG_DEBUG(this, "from", parent);
}

Environment::~Environment()
{
    while(!m_defer_task.empty()) {
        const Object& task = m_defer_task.back();
        try {
            boost::apply_visitor(eval_visitor(*this), m_defer_task.back());
        }
        catch (std::exception& e) {
            COLOG_ERROR(e.what());
        }
        catch (...)
        {
            COLOG_DEBUG("unkown exception while eval", task);
        }
        m_defer_task.pop_back();
    }
}

void   Environment::defer_task_push(const Object& task)
{
    m_defer_task.push_back(task);
}

void   Environment::defer_task_push(Object&& task)
{
    m_defer_task.emplace_back(std::move(task));
}

size_t Environment::defer_task_size() const
{
    return m_defer_task.size();
}


void   Environment::print(std::ostream& o) const
{
    o << '{';
    for (auto it = this->BaseT::begin(); it != this->BaseT::end(); ++it) {
        o << '(' << it->first << ' ' << it->second << ')';
    }
    o << '}';
}

const Object* Environment::find(const std::string& name) const
{
    const Environment* pe = this;
    const Object* ret = 0;
    do {
        auto it = pe->BaseT::find(name);
        if (it != pe->BaseT::cend()) {
            ret = &it->second;
        }
        pe = pe->m_parent;
    } while (pe && !ret);

    return ret;
}

Object* Environment::find(const std::string& name)
{
    Environment* pe = this;
    Object* ret = 0;
    do {
        auto it = pe->BaseT::find(name);
        if (it != pe->BaseT::end()) {
            ret = &it->second;
        }
        pe = pe->m_parent;
    } while (pe && !ret);

    return ret;
}

Interpreter* Environment::getInterpreter() const
{
    const Environment* pe = this;
    Interpreter* ret = 0;
    do {
        if (pe->m_parent == 0) {
            return pe->m_interpreter;
        }
        pe = pe->m_parent;
    } while (pe);
    return ret;
}

Interpreter* Environment::setInterpreter(Interpreter& interpreter)
{
    this->m_interpreter = &interpreter;
}

bool Environment::erase(const std::string& name)
{
    bool erased = false;
    Environment* pe = this;
    do {
        auto it = pe->BaseT::find(name);
        if (it != pe->BaseT::end()) {
            pe->BaseT::erase(it);
            erased = true;
        }
        pe = pe->m_parent;
    } while (pe && !erased);
    return erased;
}

Environment * Environment::ceiling(){
    Environment * p_curent = this;
    while (p_curent && p_curent->m_parent) {
        p_curent = p_curent->m_parent;
    }
    return p_curent;
}

bool Environment::operator == (const Environment& env) const
{
    // TODO
    return false;
}
bool Environment::operator < (const Environment& env) const
{
    // TODO
    return false;
}

}  // namespace varlisp
