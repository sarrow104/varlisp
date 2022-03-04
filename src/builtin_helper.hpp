#pragma once

#include <sss/colorlog.hpp>

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
//
// NOTE TODO 或许需要这样一个函数，给一个type列表，然后返回转换的结果；
// 可以转换的，这个列表对应的指针，就是非0；
// 错误信息呢？
// 最好传入1：函数名；当前参数位置；然后类型名直接显示吗？也不对；底层
// 类型名和展示给用户的类型也是不同的。

inline const Object& getAtomicValue(varlisp::Environment& env,
                                    const varlisp::Object& value, Object& tmp)
{
    COLOG_DEBUG(value);
    if (boost::apply_visitor(is_instant_visitor(env), value)) {
        return value;
    }
    tmp = boost::apply_visitor(eval_visitor(env), value);
    return tmp;
}

inline const Object& getAtomicValueUnquote(varlisp::Environment& env,
                                           const varlisp::Object& value, Object& tmp)
{
    const Object& resRef = varlisp::getAtomicValue(env, value, tmp);
    if (auto * p_list = boost::get<varlisp::List>(&resRef)) {
        if (!p_list->is_quoted()) {
            SSS_POSITION_THROW(std::runtime_error,
                               "require s-list but", resRef);
        }
        return *p_list->unquote();
    }
    return resRef;
}

inline int64_t type_id(varlisp::Environment&, const varlisp::Object& obj)
{
    switch(obj.which()) {
        case 0:
            throw std::runtime_error("query Empty typeid!"); 

        default:
            return obj.which() - 1;
    }
}

inline const varlisp::symbol* getSymbol(varlisp::Environment& env,
                 const varlisp::Object& value, Object& obj)
{
    const varlisp::symbol* p_sym = boost::get<varlisp::symbol>(&value);
    if (!p_sym) {
        obj = boost::apply_visitor(eval_visitor(env), value);
        p_sym = boost::get<varlisp::symbol>(&obj);
    }
    return p_sym;
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

struct debug_info_t
{
    const char * file;
    const char * func;
    size_t       line;
};

#ifndef DEBUG_INFO
#define DEBUG_INFO debug_info_t{__FILE__, __PRETTY_FUNCTION__, __LINE__}
#endif

template <typename T>
const char * typeName();

struct QuoteList{};

template <> inline const char * typeName<varlisp::Nill>()        { return "nil";     }
template <> inline const char * typeName<bool>()                 { return "boolean"; }
template <> inline const char * typeName<int64_t>()              { return "int64_t"; }
template <> inline const char * typeName<varlisp::string_t>()    { return "string";  }
template <> inline const char * typeName<varlisp::List>()        { return "list";    }
template <> inline const char * typeName<varlisp::symbol>()      { return "symbol";  }
template <> inline const char * typeName<varlisp::Environment>() { return "context"; }
template <> inline const char * typeName<varlisp::regex_t>()     { return "regex";   }
template <> inline const char * typeName<varlisp::gumboNode>()   { return "gumboNode"; }
template <> inline const char * typeName<varlisp::QuoteList>()   { return "s-list";  }

inline const char* readableIndex(size_t index)
{
    static char buf[30] = "";
    switch (index) {
        case 0: return "1st"; break;
        case 1: return "2nd"; break;
        case 2: return "3rd"; break;
        case 3: return "4th"; break;
        default:
            std::sprintf(buf, "%d", int(index));
            return buf;
    }
}

template <typename T>
void requireOnFaild(const void* p_value, const char* funcName, size_t index,
                    const debug_info_t& debug_info)
{
    if (!p_value) {
        std::ostringstream oss;
        oss << "(" << funcName << ": require a " << typeName<T>() << " as "
            << readableIndex(index) << " argument); from [" << debug_info.file
            << ":" << debug_info.line << "]";
        throw std::runtime_error(oss.str());
    }
}

template <typename T>
inline const T* requireTypedValue(varlisp::Environment& env,
                                  const varlisp::Object& value, Object& objTmp,
                                  const char* funcName, size_t index,
                                  const debug_info_t& debug_info)
{
    const T * p_value = getTypedValue<T>(env, value, objTmp);
    requireOnFaild<T>(p_value, funcName, index, debug_info);
    return p_value;
}

template <typename T>
inline const T* getQuotedType(varlisp::Environment& env,
                              const varlisp::Object& obj, Object& tmp)
{
    const varlisp::List* p_list = varlisp::getTypedValue<varlisp::List>(env, obj, tmp);
    if (p_list && p_list->is_quoted()) {
        return boost::get<T>(&p_list->nth(1));
    }
    return nullptr;
}

inline bool is_true(varlisp::Environment& env, const varlisp::Object& obj)
{
    Object res;
    const bool* p_bool = getTypedValue<bool>(env, obj, res);
    return p_bool && *p_bool;
}

// NOTE 注意，第二个参数是 Object的const&；这意味着，这个boost::variant的子类的参数，可以
// 自动接受其任何子类作为参数。
// 而我本来的目的是，这里必须是一个Object左值——因为我可能会原样返回这个值的引用；
// 自动类型转换，有时候，真是一个麻烦事情
const varlisp::List* getQuotedList(varlisp::Environment& env,
                                   const varlisp::Object& obj,
                                   varlisp::Object& tmp);

// NOTE 深度查询一个symbol——注意，该symbol，可以由cast等方法生成。
Object * findSymbolDeep(varlisp::Environment& env,
                        const varlisp::Object& value, Object& tmp,
                        const char * funcName = nullptr);

}  // namespace varlisp
