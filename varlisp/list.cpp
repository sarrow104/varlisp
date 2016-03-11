#include <sstream>
#include <sss/log.hpp>

#include "list.hpp"

#include "cast2double_visitor.hpp"
#include "print_visitor.hpp"
#include "strict_equal_visitor.hpp"

namespace varlisp {

    int List::length() const
    {
        int len = 0;
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

    Object List::eval(Environment& env) const
    {
        // NOTE list.eval，需要非空，不然，都会抛出异常！
        if (this->is_empty()) {
            throw std::runtime_error("() is an illegal empty application!");
        }

        int args_cnt = this->length() - 1;
        if (this->head.which() == 5) {
            // 调用函数；以this->head为函数名；this->tail为实参参数；另外有环境env；
            // 但，这不能直接调用 apply_visitor()；因为它只能作用到variant上！
            // 而List的tail[0]，还是List；就是说，我只能再次调用函数(或者函数对象)
            const varlisp::symbol& op = boost::get<varlisp::symbol>(this->head);
            static const varlisp::symbol op_add("+");
            static const varlisp::symbol op_sub("-");
            static const varlisp::symbol op_mul("*");
            static const varlisp::symbol op_div("/");
            // TODO FIXME
            // list中，内建函数，比如'+','/'等的行为，都可以被重定义、修改！
            if (op == op_add) {
                if (!args_cnt) {
                    throw std::runtime_error(op.m_data + " need at least one parameter");
                }
                double sum = 0;
                const List * p = this;
                while (p && p->head.which()) {
                    sum += boost::apply_visitor(cast2double_visitor(env), p->head);
                    if (p->tail.empty()) {
                        p = 0;
                    }
                    else {
                        p = &p->tail[0];
                    }
                }
                return Object(sum);
            }
            else if (op == op_sub) {
                if (!args_cnt) {
                    throw std::runtime_error(op.m_data + " need at least one parameter");
                }
                if (args_cnt == 1) {
                    return Object(-boost::apply_visitor(cast2double_visitor(env),
                                                        this->tail[0].head));
                }
                else {
                    double sum = boost::apply_visitor(cast2double_visitor(env), this->tail[0].head);
                    const List * p = &this->tail[0].tail[0];
                    while (p && p->head.which()) {
                        sum -= boost::apply_visitor(cast2double_visitor(env), p->head);
                        if (p->tail.empty()) {
                            p = 0;
                        }
                        else {
                            p = &p->tail[0];
                        }
                    }
                    return Object(sum);
                }
            }
            else if (op == op_mul) {
                if (args_cnt < 2) {
                    throw std::runtime_error(op.m_data + " need at least two parameter");
                }
                else {
                    double mul = boost::apply_visitor(cast2double_visitor(env), this->tail[0].head);
                    const List * p = &this->tail[0].tail[0];
                    while (mul && p && p->head.which()) {
                        mul *= boost::apply_visitor(cast2double_visitor(env), p->head);
                        if (p->tail.empty()) {
                            p = 0;
                        }
                        else {
                            p = &p->tail[0];
                        }
                    }
                    return Object(mul);
                }
            }
            else if (op == op_div) {
                if (args_cnt < 2) {
                    throw std::runtime_error(op.m_data + " need at least two parameter");
                }
                else {
                    double mul = boost::apply_visitor(cast2double_visitor(env), this->tail[0].head);
                    const List * p = &this->tail[0].tail[0];
                    while (mul && p && p->head.which()) {
                        double div = boost::apply_visitor(cast2double_visitor(env), p->head);
                        if (!div) {
                            throw std::runtime_error(" divide by zero!");
                        }
                        mul /= div;
                        if (p->tail.empty()) {
                            p = 0;
                        }
                        else {
                            p = &p->tail[0];
                        }
                    }
                    return Object(mul);
                }
            }
        }

        return Object();
    }


    void List::print(std::ostream& o) const
    {
        o << "(list";
        this->print_impl(o);
        o << ")";
    }

    void List::print_impl(std::ostream& o) const
    {
        const List * p = this;
        while (p && p->head.which()) {
            o << " ";
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
        return boost::apply_visitor(strict_equal_visitor(), lhs.head, rhs.head)
            && ((lhs.tail.empty() && rhs.tail.empty()) ||
                lhs.tail[0] == rhs.tail[0]);
    }

} // namespace varlisp
