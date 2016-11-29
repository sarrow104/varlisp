#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "eval_visitor.hpp"

#include "detail/buitin_info_t.hpp"

namespace varlisp {

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
    Object obj;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args, obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(car: need squote-List)");
    }

    return p_list->car();
}

REGIST_BUILTIN("car", 1, 1, eval_car, "(car (list item1 item2 ...)) -> item1");

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
    Object obj;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args, obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(cdr: need squote-List)");
    }
    return p_list->cdr();
}

REGIST_BUILTIN("cdr", 1, 1, eval_cdr,
               "(cdr '(list item1 item2 ...)) -> '(item2 item3 ...)");

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
    const int * p_nth = varlisp::getTypedValue<int>(env, args.head, obj1);
    if (!p_nth) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(car-nth: 1st argument must be an Integar)");
    }
    Object obj2;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args.tail[0], obj2);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(car-nth: 2nd argument must be an S-list");
    }
    p_list = p_list->next();
    if (*p_nth >= 0 && *p_nth < int(p_list->length())) {
        for (int i = 0; p_list && i < *p_nth; ++i) {
            p_list = p_list->next();
        }
        return p_list->head;
    }
    else {
        return Object{};
    }
}

REGIST_BUILTIN("car-nth", 2, 2, eval_car_nth,
               "(car-nth index '(list)) -> list[index]");

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
    const int * p_nth = varlisp::getTypedValue<int>(env, args.head, obj1);
    if (!p_nth) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(car-nth: 1st argument must be an Integar)");
    }
    Object obj2;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args.tail[0], obj2);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(car-nth: 2nd argument must be an S-list");
    }
    p_list = p_list->next();
    if (*p_nth >= 0 && *p_nth < int(p_list->length())) {
        for (int i = 0; p_list && i < *p_nth; ++i) {
            p_list = p_list->next();
        }
        return varlisp::List({varlisp::symbol{"list"}, *p_list->next()});
    }
    else {
        return varlisp::List::makeSQuoteList();
    }
}

REGIST_BUILTIN("cdr-nth", 2, 2, eval_cdr_nth,
               "(cdr-nth index '(list)) -> (list-tail[index]...)");

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
    const varlisp::List * p_list = getFirstListPtrFromArg(env, args.tail[0], obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(cons: need squote-List as the 2nd argument)");
    }
    varlisp::List ret = varlisp::List::makeSQuoteList();
    ret.append(boost::apply_visitor(eval_visitor(env), args.head));
    ret.tail[0].tail.push_back(p_list->tail[0]);
    return ret;
}

REGIST_BUILTIN("cons", 2, 2, eval_cons, "(cons 1 (cons 2 '())) -> '(1 2)");

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
    Object obj;
    const varlisp::List * p_list = getFirstListPtrFromArg(env, args, obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(length: need s-List as the 1st argument)");
    }
    return int(p_list->length() - 1);
}

REGIST_BUILTIN("length", 1, 1, eval_length,
               "(length '(list)) -> quote-list-length");

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
    const varlisp::List * p_list1 = getFirstListPtrFromArg(env, args, obj1);
    if (!p_list1) {
        SSS_POSITION_THROW(std::runtime_error, "(append: need s-List as the 1st argument)");
    }
    Object obj2;
    const varlisp::List * p_list2 = getFirstListPtrFromArg(env, args.tail[0], obj2);
    if (!p_list2) {
        SSS_POSITION_THROW(std::runtime_error, "(append: need s-List as the 2nd argument)");
    }
    varlisp::List ret = varlisp::List::makeSQuoteList();
    varlisp::List * p_ret = &ret;

    p_list1 = p_list1->next();
    while (p_list1) {
        p_ret = p_ret->next_slot();
        p_ret->head = p_list1->head;
        p_list1 = p_list1->next();
    }
    p_list2 = p_list2->next();
    while (p_list2) {
        p_ret = p_ret->next_slot();
        p_ret->head = p_list2->head;
        p_list2 = p_list2->next();
    }
    return ret;
}

REGIST_BUILTIN("append", 2, 2, eval_append,
               "(append '(list1) '(list2)) -> '(list1 list2)");

}  // namespace varlisp
