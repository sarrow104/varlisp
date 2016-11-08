#include "eval_visitor.hpp"
#include "object.hpp"

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
    // NOTE FIXME List 的第一个元素是symbol 的list!
    const varlisp::List* p_list = boost::get<const varlisp::List>(&args.head);
    if (p_list && !p_list->is_squote()) {
        obj = p_list->eval(env);
        p_list = boost::get<const varlisp::List>(&obj);
        if (p_list && !p_list->is_squote()) {
            p_list = 0;
        }
    }
    return p_list;
}
}  // namespace varlisp
