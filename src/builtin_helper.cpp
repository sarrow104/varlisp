#include "object.hpp"
#include "builtin_helper.hpp"

#include "detail/car.hpp"

namespace varlisp {

/**
 * @brief getQuotedList 
 *
 * 将args的第一个参数，获取为一个squot-list；如果是可执行的序列，则求值后再判断
 * 如果无法获得一个squot-list，则返回0；
 *
 * @param[in] env
 * @param[in] obj
 * @param[in] tmp
 *
 * @return
 */
const varlisp::List* getQuotedList(varlisp::Environment& env,
                                   const varlisp::Object& obj,
                                   varlisp::Object& tmp)
{
    const varlisp::List* p_list = varlisp::getTypedValue<varlisp::List>(env, obj, tmp);
    // NOTE 不会出现 非 s-list!，即，还需要eval的list！
    // 因为getTypedValue<>已经保证获取到的是"值"了！
    // 当然，需要注意的是各种函数，也是值！
    // 不过，if语句就不是值。为什么呢？因为if语句，是含有操作数的——
    // 已经通过了parser，构建的ifExpr结构，是一个可以被执行的语句块！
    // 至于函数，为什么是"值"，而不能执行呢？是因为，"它"需要用括号括起来，
    // 如果需要参数的话，还需要添加参数。
    if (p_list) {
        p_list = p_list->get_slist();
    }
    return p_list;
}

Object * findSymbolDeep(varlisp::Environment& env,
                        const varlisp::Object& value, Object& tmp,
                        const char * funcName)
{
    if (!funcName) {
        funcName = __func__;
    }
    const Object * p_res = &value;
    if (const auto * p_list = boost::get<varlisp::List>(&value)) {
        tmp = p_list->eval(env);
        p_res = &tmp;
    }
    const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(p_res);
    if (!p_sym) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": must require on a symbol, but ", value, ")");
    }
    return env.deep_find(p_sym->name());
}
}  // namespace varlisp
