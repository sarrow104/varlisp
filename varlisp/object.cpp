#include <iostream>
#include <stdexcept>

#include "object.hpp"
#include "print_visitor.hpp"
#include "builtin_helper.hpp"

namespace varlisp {

Object apply(Environment& env, const Object& funcObj, const List& args)
{
    Object funcTmp;
    const Object& funcRef = varlisp::getAtomicValue(env, funcObj, funcTmp);

    // 直接值(字符量，用作list的第一个元素，被求值，都是错误！)
    // 其次，是IfExpr,List，需要被eval一下；
    // 如果是Lambda，可以直接使用；
    // 如果是symbol，则进行调用尝试；
    // 不可能的值有哪些？Empty和Builtin；前者不用说了；后者只是内建函数的容
    // 器，不可能出现由表达式生成；解析到的表达式，最多只能是运算符号。

    COLOG_DEBUG(funcRef, args);
    if (const varlisp::Lambda* pl =
                 boost::get<varlisp::Lambda>(&funcRef)) {
        return pl->eval(env, args);
    }
    else if (const varlisp::Builtin* p_builtin_func =
                boost::get<varlisp::Builtin>(&funcRef))
    {
        return p_builtin_func->eval(env, args);
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, funcRef, funcRef.which(), " not callable objct");
    }
}

// std::ostream& operator<<(std::ostream& o, const varlisp::Object& obj)
// {
//     boost::apply_visitor(print_visitor(o), obj);
//     return o;
// }
}  // namespace varlisp
