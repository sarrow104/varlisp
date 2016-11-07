#include "object.hpp"
#include "builtin_helper.hpp"

#include <stdexcept>
#include <sss/util/PostionThrow.hpp>

namespace varlisp {
/**
 * @brief (car (list item1 item2 ...)) -> item1
 *
 * @param env
 * @param args
 *
 * @return 
 */
Object eval_car(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj;
    const varlisp::List * _list = getFirstListPtrFromArg(env, args, obj);

    if (!_list) {
        SSS_POSTION_THROW(std::runtime_error,
                          "(car: need List)");
    }

    if (!_list->length()) {
        SSS_POSTION_THROW(std::runtime_error,
                          "(car: contract violation expected: pair?  given: '())");
    }
    return _list->head;
}

/**
 * @brief (cdr (list item1 item2 ...)) -> (item2 item3 ...)
 *
 * @param env
 * @param args
 *
 * @return 
 *
 * TODO FIXME �����������Ҫ����eval_car���졣
 */
Object eval_cdr(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj;
    const varlisp::List * _list = getFirstListPtrFromArg(env, args, obj);

    if (!_list) {
        SSS_POSTION_THROW(std::runtime_error,
                          "(cdr: need List)");
    }
    if (!_list->length()) {
        SSS_POSTION_THROW(std::runtime_error,
                          "(cdr: contract violation expected: pair?  given: '())");
    }
    varlisp::List ret;
    List * p_list = &ret;

    _list = _list->next(); // descard the first object
    while (_list && _list->head.which()) {
        p_list = p_list->next_slot();
        p_list->head = _list->head;
        _list = _list->next();
    }
    return ret;
}

} // namespace varlisp
