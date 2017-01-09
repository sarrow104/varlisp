#ifndef __LIST_HPP_1457603019__
#define __LIST_HPP_1457603019__

#include <initializer_list>
#include <stdexcept>
#include <vector>
#include <memory>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>

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
    List(std::initializer_list<Object> l);
    // List(const Object& h, const List& t);
    List(const List& l) = default;
    // explicit List(const Object& o);
    ~List() = default;

    List& operator= (const List& l) = default;

    List(List&& m) = default;
    List& operator= (List&& l) = default;

public:
    typedef std::vector<Object> shared_t;

    typedef shared_t::iterator       iterator;
    typedef shared_t::const_iterator const_iterator;

    typedef shared_t::size_type      size_type;

protected:
    List(std::shared_ptr<shared_t> shared, size_t start, size_t len)
        : m_refer(shared), m_start(start), m_length(len)
    {}

public:
    void print(std::ostream& o) const;
    void print_impl(std::ostream& o, const List* p = 0) const;

    Object& head()
    {
        return m_refer->operator[](m_start);
    }
    const Object& head() const
    {
        return m_refer->operator[](m_start);
    }

    List tail(size_t offset = 0) const;

    iterator begin() {
        return m_refer ? m_refer->begin() + m_start : shared_t::iterator();
    }

    iterator end() {
        return m_refer ? m_refer->begin() + m_start + m_length : shared_t::iterator();
    }

    const_iterator begin() const {
        return m_refer ? m_refer->begin() + m_start : shared_t::const_iterator();
    }

    const_iterator end() const {
        return m_refer ? m_refer->begin() + m_start + m_length : shared_t::const_iterator();
    }

    size_type size() const {
        return m_length;
    }

    void clear();

    // // not used
    // void assign(const Object& value) { this->head = value; }
    // void assign(Object&& value)
    // {
    //     std::swap(this->head, std::move(value));
    // }

    // NOTE 对于quote-list(s-list)，empty?是针对第二个节点的；
    // 而对于一般的()——也就是lisp中的"程序"，empty?这个函数，有意义吗？
    // head-tail模式下，本函数，直接判断的head.which以及tail.empty();
    bool empty() const
    {
        return !m_length;
    }

    static List makeSQuoteList()
    {
        return {keywords_t{keywords_t::kw_QUOTE} ,varlisp::List{}};
    }

    template<typename ...ArgsT>
    static List makeSQuoteList(ArgsT&&... args)
    {
        return List( {keywords_t{keywords_t::kw_QUOTE}, varlisp::List{std::move(args)...}});
    }

    template<typename ...ArgsT>
    static List makeSQuoteList(const ArgsT&... args)
    {
        return List( {keywords_t{keywords_t::kw_QUOTE}, varlisp::List{args...}});
    }

    template<typename T>
    static List makeSQuoteObj(const T& o)
    {
        return List( {keywords_t{keywords_t::kw_QUOTE}, o});
    }

    template<typename T>
    static List makeSQuoteObj(const T&& o)
    {
        return List( {keywords_t{keywords_t::kw_QUOTE}, std::move(o)});
    }

    static List makeCons(const Object& o, const List& l)
    {
        List ret;
        ret.append(o);
        ret.append_list(l);
        return List( {keywords_t{keywords_t::kw_QUOTE}, std::move(ret)});
    }

    // lisp语义
    const Object * objAt(size_t i) const;
    Object * objAt(size_t i);

    Object * unquote();
    const Object * unquote() const;

    template<typename T>
    const T * unquoteType() const
    {
        if (this->is_quoted()) {
            return boost::get<T>(&this->nth(1));
        }
        return nullptr;
    }

    template<typename T>
    T * unquoteType()
    {
        if (this->is_quoted()) {
            return boost::get<T>(&this->nth(1));
        }
        return nullptr;
    }

    bool is_quoted() const;

    const List * get_slist() const;
    List * get_slist();

    const List * none_empty_squote_check() const;

    const Object& nth(size_t i) const;

    // 内部语义
    Object& nth(size_t i);

    Object& front()
    {
        return nth(0);
    }
    const Object& front() const
    {
        return nth(0);
    }

    Object& back()
    {
        return nth(m_length - 1);
    }
    const Object& back() const
    {
        return nth(m_length - 1);
    }

    // List语义
    Object car(size_t n = 0) const;
    Object cdr(size_t n = 0) const;

    Object eval(Environment& env) const;

    size_type length() const;

    // NOTE
    // 尾节点，貌似有两种状态！
    // 要么没有；tail.empty() == true;
    // 要么是一个head.which() == 0的；
    // 当然，如果某一个节点head.which() == 0，那么tail.empty()也为真！
    void append(const Object& o);
    void append(Object&& o);

    void append(varlisp::Environment& env, const Object& o);
    void append(varlisp::Environment& env, Object&& o);

    void append_list(const List& l);
    void append_list(List&& l);

    void append_list(varlisp::Environment& env, const List& l);
    void append_list(varlisp::Environment& env, List&& l);

    void push_front(const Object& o);
    void swap(List& ref);

protected:
    bool is_unique() const;
    void make_unique();

private:
    std::shared_ptr<shared_t>   m_refer;
    size_t                      m_start = 0;
    size_t                      m_length = 0;
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
