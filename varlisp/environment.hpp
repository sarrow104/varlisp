#ifndef __EVIRONMENT_HPP_1457164527__
#define __EVIRONMENT_HPP_1457164527__

#include <map>
#include <string>

#include <memory>

#include "object.hpp"

namespace varlisp {
class Interpreter;
struct Environment : private std::map<std::string, Object> {
    explicit Environment(Environment* parent = 0);
    ~Environment();

public:
    typedef std::map<std::string, Object> BaseT;
    typedef std::map<std::string, Object>::const_iterator const_iterator;
    typedef std::map<std::string, Object>::iterator iterator;

public:
    const Object* find(const std::string& name) const;
    Object* find(const std::string& name);

    using BaseT::begin;
    using BaseT::end;
    using BaseT::cbegin;
    using BaseT::cend;
    using BaseT::size;
    using BaseT::empty;

    Object& operator [](const std::string& name);

    bool erase(const std::string& name);

    Environment * parent() const {
        return m_parent;
    }
    Environment * ceiling();
    void   defer_task_push(const Object& task);
    void   defer_task_push(Object&& task);
    size_t defer_task_size() const;
    void   print(std::ostream& ) const;

    Object eval(Environment& env) const
    {
        return *this;
    }

    bool operator == (const Environment& env) const;
    bool operator < (const Environment& env) const;

private:
    Environment*        m_parent;
    std::vector<Object> m_defer_task;
};

inline std::ostream& operator<<(std::ostream& o, const Environment& e)
{
    e.print(o);
    return o;
}
}  // namespace varlisp

#endif /* __EVIRONMENT_HPP_1457164527__ */
