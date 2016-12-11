#ifndef __EVIRONMENT_HPP_1457164527__
#define __EVIRONMENT_HPP_1457164527__

#include <map>
#include <string>

#include <memory>

#include "object.hpp"

namespace varlisp {
class Interpreter;
// std::map<std::string, std::pair<Object, bool>>
//  <name - Object is_const>
struct property_t
{
    property_t() : is_const(false) {}
    property_t(bool c) : is_const(c) {}
    bool is_const = false;
};

struct Environment : private std::map<std::string, std::pair<Object, property_t>> {
    explicit Environment(Environment* parent = 0);
    ~Environment();
    // Environment(const Environment& ref);
    // Environment& operator = (const Environment& ref);

public:
    typedef std::map<std::string, std::pair<Object, property_t> > BaseT;
    typedef BaseT::const_iterator                                 const_iterator;
    typedef BaseT::iterator                                       iterator;

public:
    const Object* find(const std::string& name) const;
    Object* find(const std::string& name);

    const Object* deep_find(const std::string& name) const;
    Object* deep_find(const std::string& name);

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
