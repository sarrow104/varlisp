#include "list.hpp"
#include "object.hpp"

#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

#include <sss/colorlog.hpp>
#include <sss/log.hpp>
#include <sss/util/PostionThrow.hpp>

#include <sstream>
#include <stdexcept>

namespace varlisp {

List::List(const List& l)
{
    const List* p_l = &l;
    List* p_this = this;
    while (p_l && p_l->head.which()) {
        p_this->head = p_l->head;
        p_l = p_l->next();
        if (p_l) {
            p_this = p_this->next_slot();
        }
    }
}

List& List::operator=(const List& l)
{
    if (this != &l) {
        List tmp{l};
        this->swap(tmp);
    }
    return *this;
}

size_t List::length() const
{
    size_t len = 0;
    const List* p = this;
    while (p && p->head.which()) {
        len++;
        p = p->next();
    }
    return len;
}

void List::append(const Object& o)
{
    List* p = this;
    while (p->head.which() && !p->tail.empty()) {
        p = &p->tail[0];
    }
    if (!p->head.which()) {
        p->head = o;
    }
    else {
        p->tail.push_back(List(o));
    }
    // std::cout << "after:" << *this << std::endl;
}

void List::append(Object&& o)
{
    List* p = this;
    while (p->head.which() && !p->tail.empty()) {
        p = &p->tail[0];
    }
    if (!p->head.which()) {
        p->head = std::move(o);
    }
    else {
        p->tail.push_back(List(std::move(o)));
    }
}

void List::swap(List& ref)
{
    std::swap(this->head, ref.head);
    std::swap(this->tail, ref.tail);
}

void List::push_front(const Object& o)
{
    List tmp;
    tmp.swap(*this);
    this->head = o;
    tail.emplace_back(tmp);
}

Object eval_impl(Environment& env, const Object& funcObj, const List& args)
{
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, funcObj);
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, args);
    if (const varlisp::symbol* ps = boost::get<varlisp::symbol>(&funcObj)) {
        std::string name = (*ps).m_data;
        Object* p_func = env.find(name);
        if (!p_func) {
            SSS_POSITION_THROW(std::runtime_error, "Application ", name,
                              " not exist.");
        }
        const Object& invokable = *p_func;

        if (const varlisp::Builtin* p_builtin_func =
                boost::get<varlisp::Builtin>(&invokable)) {
            return p_builtin_func->eval(env, args);
        }
        else if (const varlisp::Lambda* pl =
                     boost::get<varlisp::Lambda>(&invokable)) {
            return pl->eval(env, args);
        }
        else {
            SSS_POSITION_THROW(std::runtime_error, "Application ", name,
                              " not invokable.");
        }
    }
    else if (const varlisp::Lambda* pl =
                 boost::get<varlisp::Lambda>(&funcObj)) {
        return pl->eval(env, args);
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, funcObj, " not callable objct");
    }
}

Object List::eval(Environment& env) const
{
    if (this->is_squote()) {
        return *this;
    }
    // NOTE list.eval，需要非空，不然，都会抛出异常！
    if (this->is_empty()) {
        throw std::runtime_error("() is an illegal empty application!");
    }
    // 晕！
    //
    // 理论上，需要先对每一个元素进行eval；然后再对新的list，应用eval！
    //

    varlisp::List nil;

    Object funcObj;

    // 直接值(字符量，用作list的第一个元素，被求值，都是错误！)
    // 其次，是IfExpr,List，需要被eval一下；
    // 如果是Lambda，可以直接使用；
    // 如果是symbol，则进行调用尝试；
    // 不可能的值有哪些？Empty和Builtin；前者不用说了；后者只是内建函数的容
    // 器，不可能出现由表达式生成；解析到的表达式，最多只能是运算符号。
    if (const varlisp::List* pl = boost::get<varlisp::List>(&this->head)) {
        funcObj = pl->eval(env);
    }
    else if (const varlisp::IfExpr* pi =
                 boost::get<varlisp::IfExpr>(&this->head)) {
        funcObj = pi->eval(env);
    }

    return eval_impl(env, funcObj.which() ? funcObj : this->head,
                     this->tail.empty() ? nil : this->tail[0]);
}

void List::print(std::ostream& o) const
{
    o << "(";
    this->print_impl(o);
    o << ")";
}

void List::print_impl(std::ostream& o) const
{
    const List* p = this;
    bool is_first = true;
    while (p && p->head.which()) {
        if (is_first) {
            is_first = false;
        }
        else {
            o << " ";
        }
        boost::apply_visitor(print_visitor(o), p->head);
        p = p->next();
    }
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

bool List::is_squote() const
{
    if (this->head.which()) {
        const varlisp::symbol* p_symbol =
            boost::get<const varlisp::symbol>(&this->head);
        if (p_symbol && *p_symbol == varlisp::symbol("list")) {
            return true;
        }
    }
    return false;
}

void List::none_empty_squote_check() const
{
    if (!this->is_squote()) {
        SSS_POSITION_THROW(std::runtime_error, "need squote-list; but", *this);
    }
    else if (this->length() < 2) {
        SSS_POSITION_THROW(std::runtime_error, "need at least one object");
    }
}

Object List::car() const
{
    none_empty_squote_check();
    return this->next()->head;
}

Object List::cdr() const
{
    none_empty_squote_check();
    return List({varlisp::symbol{"list"}, *(this->next()->next())});
}

}  // namespace varlisp
