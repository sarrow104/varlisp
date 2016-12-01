#ifndef __LIST_HPP_1457603019__
#define __LIST_HPP_1457603019__

#include <initializer_list>
#include <stdexcept>
#include <vector>

#include "object.hpp"

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
    List(const List& l);
    explicit List(const Object& o) : head(o) {}
    ~List() = default;

    List& operator= (const List& l);

    List(List&& m) = default;
    List& operator= (List&& l) = default;

    Object head;
    // NOTE 这本质上是 optional，而不是一个vector；我只会用到第一个元素；
    std::vector<List> tail;

    void print(std::ostream& o) const;
    void print_impl(std::ostream& o, const List* p = 0) const;

    List* next_slot()
    {
        if (this->tail.empty()) {
            this->tail.push_back(varlisp::List());
        }
        return &tail[0];
    }

    void clean();

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

    static List makeList(std::initializer_list<Object> l)
    {
        varlisp::List ret;
        varlisp::List *p_list = &ret;
        for (auto& item : l) {
            p_list->head = item;
            p_list = p_list->next_slot();
        }
        return ret;
    }

    static List makeSQuoteList(std::initializer_list<Object> l)
    {
        return makeList({Object{varlisp::symbol("list")}, makeList(std::move(l))});
    }

    static List makeSQuoteList()
    {
        return makeList({Object{varlisp::symbol("list")}});
    }

    bool is_squote() const;

    void none_empty_squote_check() const;

    Object car() const;
    Object cdr() const;

    Object eval(Environment& env) const;

    size_t length() const;

    // NOTE
    // 尾节点，貌似有两种状态！
    // 要么没有；tail.empty() == true;
    // 要么是一个head.which() == 0的；
    // 当然，如果某一个节点head.which() == 0，那么tail.empty()也为真！
    void append(const Object& o);
    void append(Object&& o);

    void push_front(const Object& o);
    void swap(List& ref);
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
