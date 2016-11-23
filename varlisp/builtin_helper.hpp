#pragma once

#include "object.hpp"

#include "eval_visitor.hpp"
#include "is_instant_visitor.hpp"

namespace varlisp {
struct List;
struct Environment;

// boost::variant 没有直接获取子类型下标的办法。或许boost::mpl可以解决。
//! http://stackoverflow.com/questions/9313108/construct-a-boost-variant-containing-a-value-of-the-nth-type-in-the-variant-type
//
// 或许boost::typeIndex也是一个方向。——好像不行。这个是对类型，
// 做一个全局的信息。并且不用依赖C++的rtti。
//
//! http://www.boost.org/doc/libs/1_56_0/doc/html/boost_typeindex.html
//
// 江南大大则说，可以直接用他的Variant。不过，就是不知道这个版本，是否支持recursive。
//! http://www.cnblogs.com/qicosmos/p/3416432.html
//! http://www.cnblogs.com/qicosmos/p/3559424.html
//
// 至于 boost::variant的理解，可以看：
//! http://blog.csdn.net/hjing1988/article/details/46945925
//
// 最后，我其实不用那么介意eval_visitor中对象的创建——
// 唯一需要优化的是s-list。其他的类型，速度都不慢。
inline const Object& getAtomicValue(varlisp::Environment& env,
                                    const varlisp::Object& value, Object& tmp)
{
    if (boost::apply_visitor(is_instant_visitor(env), value)) {
        return value;
    }
    tmp = boost::apply_visitor(eval_visitor(env), value);
    return tmp;
}

inline int typedid(varlisp::Environment&, const varlisp::Object& obj)
{
    switch(obj.which()) {
        case 0:
            throw std::runtime_error("query Empty typeid!"); 

        default:
            return obj.which() - 1;
    }
}

template <typename T>
inline const T* getTypedValue(varlisp::Environment& env,
                              const varlisp::Object& value, Object& obj)
{
    // NOTE 从Object中取某个类型的值，有三种情况：
    // 1. 立即数
    // 2. 变量；需要通过env获取；
    // 3. 可执行的序列，需要eval；
    //
    // 此外，2、3可能会发生递归调用；
    // 于是，分别处理情况1，还有合并处理情况2,3。
    //
    // 不过需要注意的是 s-list，不应该执行，不再需要eval_visitor()
    // 所以，最好一个有一个xxx_visitor，可以返回不再能eval的对象。
    // 我的想法是能返回引用，最好就引用。
    // 为此，还得传入一个Object&，用来存放临时的值。
    // 不过，貌似visitor办不到——因为，进入visitor的operator()函数之后，
    // 已经变为具体的类型了。想操作拥有这个对象的Object&元素，是不可能的了。
    return boost::get<const T>(&getAtomicValue(env, value, obj));
}

inline bool is_true(varlisp::Environment& env, const varlisp::Object& obj)
{
    Object res;
    const bool* p_bool = getTypedValue<bool>(env, obj, res);
    return p_bool && *p_bool;
}

const varlisp::List* getFirstListPtrFromArg(varlisp::Environment& env,
                                            const varlisp::List& args,
                                            Object& tmp);
}  // namespace varlisp
