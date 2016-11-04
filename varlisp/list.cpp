#include <sstream>
#include <sss/log.hpp>

#include "list.hpp"
#include "object.hpp"

#include "cast2double_visitor.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

namespace varlisp {

    size_t List::length() const
    {
        size_t len = 0;
        const List * p = this;
        while (p && p->head.which()) {
            len++;
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
        return len;
    }

    void List::append(const Object& o)
    {
        // std::ostringstream oss;
        // oss << __func__ << " ";
        // boost::apply_visitor(print_visitor(oss), o);
        // oss << std::endl;
        // std::cout << oss.str();
        // std::cout << "before:" << *this << std::endl;

        List * p = this;
        while (p->head.which() && !p->tail.empty()) {
            p = &p->tail[0];
        }
        if (!p->head.which()) {
            p->head = o;
        }
        else {
            p->tail.push_back(List(std::move(o)));
        }
        // std::cout << "after:" << *this << std::endl;
    }

    void List::append(Object&& o)
    {
        // std::ostringstream oss;
        // oss << __func__ << " ";
        // boost::apply_visitor(print_visitor(oss), o);
        // oss << std::endl;
        // std::cout << oss.str();
        // std::cout << "before:" << *this << std::endl;

        List * p = this;
        while (p->head.which() && !p->tail.empty()) {
            p = &p->tail[0];
        }
        if (!p->head.which()) {
            p->head = std::move(o);
        }
        else {
            p->tail.push_back(List(std::move(o)));
        }
        // std::cout << "after:" << *this << std::endl;
    }

    Object eval_impl(Environment& env, const Object& funcObj, const List& args)
    {
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, funcObj);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, args);
        if (const varlisp::symbol * ps = boost::get<varlisp::symbol>(&funcObj)) {
            if (*ps == varlisp::symbol("list")) {
                return args;
            }

            std::string name = (*ps).m_data;
            Object * p_func = env.find(name);
            if (!p_func) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "Application ", name , " not exist.");
            }
            const Object& invokable = *p_func;

            if (const varlisp::Builtin *p_builtin_func = boost::get<varlisp::Builtin>(&invokable)) {
                return p_builtin_func->eval(env, args);
            }
            else if (const varlisp::Lambda *pl = boost::get<varlisp::Lambda>(&invokable))
            {
                return pl->eval(env, args);
            }
            else {
                SSS_POSTION_THROW(std::runtime_error,
                                  "Application ", name , " not invokable.");
            }
        }
        else if (const varlisp::Lambda * pl = boost::get<varlisp::Lambda>(&funcObj)) {
            return pl->eval(env, args);
        }
        else {
            std::ostringstream oss;
            boost::apply_visitor(print_visitor(oss), funcObj);
            SSS_POSTION_THROW(std::runtime_error,
                              oss.str() , " not callable objct");
        }
    }

    Object List::eval(Environment& env) const
    {
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
        if (const varlisp::List * pl = boost::get<varlisp::List>(&this->head)) {
            funcObj = pl->eval(env);
        }
        else if (const varlisp::IfExpr * pi = boost::get<varlisp::IfExpr>(&this->head)) {
            funcObj = pi->eval(env);
        }

        return eval_impl(env,
                         funcObj.which() ? funcObj : this->head,
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
        const List * p = this;
        bool is_first = true;
        while (p && p->head.which()) {
            if (is_first) {
                is_first = false;
            }
            else {
                o << " ";
            }
            boost::apply_visitor(print_visitor(o), p->head);
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
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

} // namespace varlisp
