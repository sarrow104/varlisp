#ifndef __LIST_HPP_1457603019__
#define __LIST_HPP_1457603019__

#include "object.hpp"

#include <stdexcept>
#include <vector>

namespace varlisp {

struct Environment;

// 如何表示nil的list呢？很简单，默认构造函数就是！
// list的结构，始终是一个大麻烦；
// 从逻辑上，它是一个递归定义；
// 一个list有两部分；分别是"头"，和"尾"；前者，是当前list的第一个元素；后者，则
// 是当前list的后续list所组成的另外一个list
//
// 是否需要nil？
//
// 如果，让 head==EMPTY, tail==vector.empty() 来表示nil节点的话；是不能够，再附
// 加一个nil的！
struct List {
    List() {}
    List(const Object& h, const List& t) : head(h) { tail.push_back(t); }
    explicit List(const Object& o) : head(o) {}
    Object head;
    std::vector<List> tail;

    void print(std::ostream& o) const;
    void print_impl(std::ostream& o) const;

    List* next_slot()
    {
        List* p_list = this;
        if (p_list->head.which()) {
            p_list->tail.push_back(varlisp::List());
            p_list = &p_list->tail[0];
        }
        return p_list;
    }

    const List* next() const { return this->tail.empty() ? 0 : &this->tail[0]; }
    void assign(const Object& value) { this->head = value; }
    // void assign(Object&& value)
    // {
    //     std::swap(this->head, std::move(value));
    // }

    bool is_empty() const
    {
        return this->head.which() == 0 && this->tail.empty();
    }

    Object eval(Environment& env) const;

    size_t length() const;

    // NOTE
    // 尾节点，貌似有两种状态！
    // 要么没有；tail.empty() == true;
    // 要么是一个head.which() == 0的；
    // 当然，如果某一个节点head.which() == 0，那么tail.empty()也为真！
    void append(const Object& o);
    void append(Object&& o);
};

inline std::ostream& operator<<(std::ostream& o, const List& d)
{
    d.print(o);
    return o;
}

bool operator==(const List& lhs, const List& rhs);
bool operator<(const List& lhs, const List& rhs);
}  // namespace varlisp

#endif /* __LIST_HPP_1457603019__ */
