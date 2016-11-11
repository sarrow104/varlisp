#include "object.hpp"
#include "builtin_helper.hpp"

namespace varlisp {

/**
 * @brief getFirstListPtrFromArg 
 *
 * 讲args的第一个参数，获取为一个squot-list；如果是可执行的序列，则求值后再判断
 * 如果无法获得一个squot-list，则返回0；
 *
 * @param[in] env
 * @param[in] args
 * @param[in] obj
 *
 * @return
 */
const varlisp::List* getFirstListPtrFromArg(varlisp::Environment& env,
                                            const varlisp::List& args,
                                            Object& obj)
{
    const varlisp::List* p_list = varlisp::getTypedValue<varlisp::List>(env, args.head, obj);
    // NOTE 不会出现 非 s-list!，即，还需要eval的list！
    // 因为getTypedValue<>已经保证获取到的是"值"了！
    // 当然，需要注意的是各种函数，也是值！
    // 不过，if语句就不是值。为什么呢？因为if语句，是含有操作数的——
    // 已经通过了parser，构建的ifExpr结构，是一个可以被执行的语句块！
    // 至于函数，为什么是"值"，而不能执行呢？是因为，"它"需要用括号括起来，
    // 如果需要参数的话，还需要添加参数。
    if (p_list && !p_list->is_squote()) {
        SSS_POSTION_THROW(std::runtime_error, "need eval list error!");
        // p_list = 0;
    }
    return p_list;
}
}  // namespace varlisp
