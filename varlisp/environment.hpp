#ifndef __EVIRONMENT_HPP_1457164527__
#define __EVIRONMENT_HPP_1457164527__

#include <map>
#include <string>

#include <memory>

#include "object.hpp"

namespace varlisp {
class Interpreter;
struct Environment : public std::map<std::string, Object> {
    typedef std::map<std::string, Object> BaseT;
    typedef std::map<std::string, Object>::const_iterator const_iterator;
    typedef std::map<std::string, Object>::iterator iterator;

    // TODO FIXME 不知道为什么；
    // 若返回值类型是iterator；那么，按名字，在父环境中才找到对象，并返回父
    // 环境的iterator的话，
    // 返回之前，都可以看到值能指向正确的对象；
    // 调用方，在接受iterator，之后，再检查，值就变化了；
    // 不得已，修改为返回指针的形式；
    const Object* find(const std::string& name) const;
    Object* find(const std::string& name);
    // Object& operator[](const std::string& name);

    Interpreter* getInterpreter() const;
    Interpreter* setInterpreter(Interpreter& interpreter);

    using BaseT::begin;
    using BaseT::end;
    using BaseT::cbegin;
    using BaseT::cend;
    using BaseT::operator[];

    explicit Environment(Environment* parent = 0);

private:
    Environment* m_parent;
    Interpreter* m_interpreter;
};
}  // namespace varlisp

#endif /* __EVIRONMENT_HPP_1457164527__ */
