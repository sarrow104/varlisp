#include <stdexcept>
#include <algorithm>
#include <iterator>

#include <sss/util/PostionThrow.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../eval_visitor.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/list_iterator.hpp"

namespace varlisp {

REGIST_BUILTIN("car", 1, 1, eval_car, "(car (list item1 item2 ...)) -> item1");

/**
 * @brief (car (list item1 item2 ...)) -> item1
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_car(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "car";
    Object obj;
    const varlisp::List* p_list = getQuotedList(env, detail::car(args), obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": need squote-List)");
    }

    return p_list->nth(0);
}

REGIST_BUILTIN("cdr", 1, 1, eval_cdr,
               "(cdr '(list item1 item2 ...)) -> '(item2 item3 ...)");

/**
 * @brief (cdr '(list item1 item2 ...)) -> '(item2 item3 ...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_cdr(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "cdr";
    Object obj;
    const varlisp::List* p_list =
        varlisp::getTypedValue<varlisp::List>(env, detail::car(args), obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": need squote-List)");
    }
    return p_list->cdr();
}

REGIST_BUILTIN("car-nth", 2, 2, eval_car_nth,
               "(car-nth index '(list)) -> list[index]");

/**
 * @brief
 *      (car-nth index '(list)) -> list[index]
 *      NOTE: index start from 0
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_car_nth(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const int64_t * p_nth = varlisp::getTypedValue<int64_t>(env, detail::car(args), obj1);
    if (!p_nth) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(car-nth: 1st argument must be an Integar)");
    }
    Object obj2;
    const varlisp::List* p_list = varlisp::getQuotedList(env, detail::cadr(args), obj2);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(car-nth: 2nd argument must be an S-list");
    }
    return p_list->nth(*p_nth);
}

REGIST_BUILTIN("cdr-nth", 2, 2, eval_cdr_nth,
               "(cdr-nth index '(list)) -> (list-tail[index]...)");

/**
 * @brief
 *      (cdr-nth index '(list)) -> (list-tail[index]...)
 *      NOTE: index start from 0
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_cdr_nth(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const int64_t * p_nth = varlisp::getTypedValue<int64_t>(env, detail::car(args), obj1);
    if (!p_nth) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(car-nth: 1st argument must be an Integar)");
    }
    Object obj2;
    const varlisp::List* p_list =
        varlisp::getTypedValue<varlisp::List>(env, detail::cadr(args), obj2);
    return p_list->cdr(*p_nth);
}

REGIST_BUILTIN("cons", 2, 2, eval_cons, "(cons 1 (cons 2 '())) -> '(1 2)");

/**
 * @brief
 *    (cons 1 (cons 2 '())) -> '(1 2)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_cons(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj;
    const varlisp::List * p_list = varlisp::getQuotedList(env, detail::cadr(args), obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(cons: need squote-List as the 2nd argument)");
    }
    Object tmp;
    const Object& headRef = varlisp::getAtomicValue(env, detail::car(args), tmp);
    if (p_list->empty()) {
        return varlisp::List::makeSQuoteList(headRef);
    }
    else {
        // NOTE FIXME
        // 第二个参数，如果是list，需要拆开，重组！
        // 如果是单值，则是 dot 并列结构——
        return varlisp::List::makeCons(headRef, *p_list);
    }
}

REGIST_BUILTIN("length", 1, 1, eval_length,
               "(length '(list)) -> quote-list-length");

/**
 * @brief
 *    (length '(list)) -> quote-list-length
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_length(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "length";
    Object obj;
    if (const varlisp::List * p_list = varlisp::getQuotedList(env, detail::car(args), obj)) {
        return int64_t(p_list->length());
    }
    else if (const varlisp::Environment* p_env =
                 varlisp::getTypedValue<varlisp::Environment>(env, detail::car(args),
                                                              obj)) {
        return int64_t(p_env->size());
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": not support on this object ", detail::car(args), ")");
    }
}

REGIST_BUILTIN("empty?", 1, 1, eval_empty_q,
               "(empty? '(list)) -> boolean");

/**
 * @brief
 *    (empty? '(list)) -> boolean
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_empty_q(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "empty?";
    Object obj;
    if (const varlisp::List * p_list = getQuotedList(env, detail::car(args), obj)) {
        return p_list->empty();
    }
    else if (const varlisp::Environment* p_env =
                 varlisp::getTypedValue<varlisp::Environment>(env, detail::car(args),
                                                              obj)) {
        return p_env->empty();
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": not support on this object ", detail::car(args), ")");
    }
}

REGIST_BUILTIN("append", 2, 2, eval_append,
               "(append '(list1) '(list2)) -> '(list1 list2)");

/**
 * @brief
 *    (append '(list1) '(list2)) -> '(list1 list2)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_append(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const varlisp::List * p_list1 = getQuotedList(env, detail::car(args), obj1);
    if (!p_list1) {
        SSS_POSITION_THROW(std::runtime_error, "(append: need s-List as the 1st argument)");
    }
    Object obj2;
    const varlisp::List * p_list2 = getQuotedList(env, detail::cadr(args), obj2);
    if (!p_list2) {
        SSS_POSITION_THROW(std::runtime_error, "(append: need s-List as the 2nd argument)");
    }
    varlisp::List ret = varlisp::List::makeSQuoteList();

    auto back_it = detail::list_back_inserter<Object>(ret);
    // FIXME delete！ list_object_const_iterator_t
    for (auto read_it = p_list1->begin(); read_it != p_list1->end();) {
        *back_it++ = *read_it++;
    }
    for (auto read_it = p_list2->begin(); read_it != p_list2->end();) {
        *back_it++ = *read_it++;
    }
    // std::copy(detail::list_object_const_iterator_t(p_list1->next()), detail::list_object_const_iterator_t(), back_it);
    // std::copy(detail::list_object_const_iterator_t(p_list2->next()), detail::list_object_const_iterator_t(), back_it);
    return ret;
}

}  // namespace varlisp
