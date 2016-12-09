#include "list.hpp"

#include <memory>
#include <sstream>
#include <stdexcept>
#include <iterator>

#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/log.hpp>
#include <sss/util/PostionThrow.hpp>

#include "object.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"
#include "builtin_helper.hpp"
#include "detail/car.hpp"
#include "keyword_t.hpp"

namespace varlisp {

// 之前，head,tail的方式，如果是s-list，第一个元素是list，
// 相当于联合考虑"list"，与后续的tail。
// 现在，修改为 quote，Object 方式之后，同样需要整体考虑它们。
// 比如s-list的append等算法
//
// 另外，if,define等关键字，也应该纳入这个考虑的集合中。
// 为了保存额外的信息，keywords_t还应该含有其他的存储信息，
// 以便保存跳转节点信息。
//

List::List(std::initializer_list<Object> l)
    : m_refer(std::make_shared<shared_t>(l)), m_start(0), m_length(l.size())
{
}

// List::List(const Object& h, const List& t)
// {
//     // 相当于push_front
//     // FIXME 如果t本身，也是(quote ，应该怎么办？）
//     m_refer = std::make_shared<shared_t>();
//     m_refer->reserve(1 + t.size());
//     m_refer->emplace_back(h);
//     std::copy(t.begin(), t.end(), std::back_inserter(*m_refer));
//     m_start = 0;
//     m_length = 1 + t.size();
// }

size_t List::length() const
{
    return this->m_length;
}

void List::make_unique()
{
    // FIXME 多线程安全
    if (!this->m_refer) {
        m_refer = std::make_shared<shared_t>();
        m_start = 0;
        m_length = 0;
    }
    else if (!this->m_refer.unique()) {
        if (m_length > 0) {
            auto tmp_refer = std::make_shared<shared_t>(m_refer->begin() + m_start,
                                                        m_refer->begin() + m_start + m_length);
            m_refer = tmp_refer;
        }
        else {
            m_refer = std::make_shared<shared_t>();
        }
    }
}

namespace detail {

const List *get_list(const List& l)
{
    // TODO FIXME 对于'1这种单值，可能有问题
    const List * p_l = l.get_slist();
    if (!p_l) {
        p_l = &l;
    }
    return p_l;
}

List * get_list(List & l)
{
    return const_cast<List*>(static_cast<const List&>(l).get_slist());
}
} // namespace detail

void List::append(const Object& o)
{
    this->make_unique();
    m_refer->push_back(o);
    m_length++;
}

void List::append(Object&& o)
{
    this->make_unique();
    m_refer->emplace_back(std::move(o));
    m_length++;
}

void List::append_list(const List& l)
{
    this->make_unique();
    m_refer->reserve(this->size() + l.size());
    for (auto it = l.begin(); it != l.end(); ++it) {
        m_refer->emplace_back(*it);
    }
    m_length += l.size();
}

void List::append_list(List&& l)
{
    this->append_list(static_cast<const List&>(l));
    l.clear();
}

void List::swap(List& ref)
{
    std::swap(this->m_refer, ref.m_refer);
    std::swap(this->m_start, ref.m_start);
    std::swap(this->m_length, ref.m_length);
}

void List::push_front(const Object& o)
{
    this->make_unique();
    m_refer->resize(m_refer->size() + 1);
    std::move(m_refer->begin(), m_refer->end(), m_refer->begin() + 1);
    m_refer->operator[](0) = o;
}

Object eval_impl(Environment& env, const Object& funcObj, const List& args)
{
    COLOG_DEBUG(funcObj, args);
    if (const varlisp::Lambda* pl =
                 boost::get<varlisp::Lambda>(&funcObj)) {
        return pl->eval(env, args);
    }
    else if (const varlisp::Builtin* p_builtin_func =
                boost::get<varlisp::Builtin>(&funcObj))
    {
        return p_builtin_func->eval(env, args);
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, funcObj, " not callable objct");
    }
}

Object List::eval(Environment& env) const
{
    COLOG_DEBUG(*this);
    if (this->is_squote()) {
        return *this;
    }
    // NOTE list.eval，需要非空，不然，都会抛出异常！
    if (this->empty()) {
        throw std::runtime_error("() is an illegal empty application!");
    }

    varlisp::List nil;

    Object funcTmp;
    const Object& funcRef = varlisp::getAtomicValue(env, this->front(), funcTmp);

    // 直接值(字符量，用作list的第一个元素，被求值，都是错误！)
    // 其次，是IfExpr,List，需要被eval一下；
    // 如果是Lambda，可以直接使用；
    // 如果是symbol，则进行调用尝试；
    // 不可能的值有哪些？Empty和Builtin；前者不用说了；后者只是内建函数的容
    // 器，不可能出现由表达式生成；解析到的表达式，最多只能是运算符号。

    COLOG_DEBUG(funcRef);

    return eval_impl(env, funcRef, this->tail());
}

void List::print(std::ostream& o) const
{
    if (this->is_squote()) {
        const List * p_tail = nullptr;
        p_tail = boost::get<varlisp::List>(&this->nth(1));
        if (p_tail) {
            o << "[";
            this->print_impl(o, p_tail);
            o << "]";
        }
        else {
#if 1
            o << '(' << "quote ";
            boost::apply_visitor(print_visitor(o), this->nth(1));
            o << ')';
#else
            // 或者
            o << '\'';
            boost::apply_visitor(print_visitor(o), this->nth(1));
#endif
        }
    }
    else {
        o << "(";
        this->print_impl(o);
        o << ")";
    }
}

void List::print_impl(std::ostream& o, const List * p) const
{
    if (p == nullptr) {
        p = this;
    }
    bool is_first = true;
    for (const auto& obj : *p) {
        if (is_first) {
            is_first = false;
        }
        else {
            o << " ";
        }
        boost::apply_visitor(print_visitor(o), obj);
    }
}

/**
 * @brief clear 确保如果某节点head.which() == 0的时候，它没有tail！
 */
void List::clear()
{
    m_refer.reset();
    m_start = 0;
    m_length = 0;
}

bool operator==(const List& lhs, const List& rhs)
{
    // return boost::apply_visitor(strict_equal_visitor(), lhs.head, rhs.head)
    //     && ((lhs.tail.empty() && rhs.tail.empty()) ||
    //         lhs.tail[0] == rhs.tail[0]);
    return false;
}

bool operator<(const List& lhs, const List& rhs)
{
    // TODO FIXME
    return false;
}

const Object * List::objAt(size_t i) const
{
    const List* p = none_empty_squote_check();

    if (p && p->size() > i) {
        return &p->nth(i);
    }
    return nullptr;
}

Object * List::objAt(size_t i)
{
    List* p = const_cast<List*>(none_empty_squote_check());

    if (p && p->size() > i) {
        return &p->nth(i);
    }
    return nullptr;
}

bool List::is_squote() const
{
    if (this->m_length >= 1) {
        const auto * p_key =
            boost::get<varlisp::keywords_t>(&(*this->begin()));
        if (!p_key || p_key->type() != keywords_t::kw_QUOTE) {
            return false;
        }
        if (this->m_length != 2) {
            SSS_POSITION_THROW(std::runtime_error,
                               SSS_VALUE_MSG(this->m_length));
        }
        return true;
    }
    return false;
}

// NOTE FIXME is_squote() 函数语义变化。
// 之前，我不支持'操作符，因此，is_squote()，就能区分是'()还是()；
// 而现在，支持'操作符的话，而我是利用vector来模拟这个嵌套关系。
// 于是，我需要更明确的判断函数，is_slist()，甚至
const List * List::get_slist() const
{
    const List * p_slist = nullptr;
    if (this->is_squote()) {
        p_slist = boost::get<varlisp::List>(&this->nth(1));
    }
    // if (p_slist) {
    //     COLOG_DEBUG(*this, *p_slist);
    // }
    // else {
    //     COLOG_DEBUG(*this);
    // }
    return p_slist;
}

List * List::get_slist()
{
    return const_cast<List*>(const_cast<const List*>(this)->get_slist());
}

const List * List::none_empty_squote_check() const
{
    if (!this->is_squote()) {
        SSS_POSITION_THROW(std::runtime_error, "need squote-list; but", *this);
    }
    else {
        const auto * p_l = boost::get<varlisp::List>(&this->nth(1));
        if (!p_l) {
            SSS_POSITION_THROW(std::runtime_error, "expect a list; but", this->nth(1));
        }
        if (p_l->size() == 0) {
            SSS_POSITION_THROW(std::runtime_error, "need at least one object");
        }
        return p_l;
    }
}

const Object& List::nth(size_t i) const
{
    if (i >= this->size()) {
        SSS_POSITION_THROW(std::runtime_error,
                           "require ", i, "th element; but only ", m_length);
    }
    return *(this->begin() + i);
}

Object& List::nth(size_t i)
{
    if (i >= this->size()) {
        SSS_POSITION_THROW(std::runtime_error,
                           "require ", i, "th element; but only ", m_length, "element(s).");
    }
    return *(this->begin() + i);
}

List List::tail(size_t offset) const
{
    if (!m_length) {
        SSS_POSITION_THROW(std::runtime_error, "empty list");
    }
    if (offset >= m_length) {
        SSS_POSITION_THROW(std::runtime_error,
                           "require ", offset, "th tail; but only ", m_length, "element(s).");
    }
    ++offset;
    if (offset == m_length) {
        return List();
    }
    else {
        return List(m_refer, m_start + offset , m_length - offset);
    }
}

// 注意，这是lisp语义的car！
Object List::car(size_t n) const
{
    auto p_l = none_empty_squote_check();
    return p_l->nth(n);
}

// 注意，这是lisp语义的car！
Object List::cdr(size_t n) const
{
    // FIXME 是否需要s-list？
    auto p_l = none_empty_squote_check();
    COLOG_DEBUG(*p_l);
    return List({
        varlisp::keywords_t{keywords_t::kw_QUOTE},
        p_l->tail(n)});
}

}  // namespace varlisp
