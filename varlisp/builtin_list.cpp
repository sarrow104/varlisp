#include "builtin_helper.hpp"
#include "eval_visitor.hpp"
#include "object.hpp"

#include <sss/util/PostionThrow.hpp>
#include <stdexcept>

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
        SSS_POSTION_THROW(std::runtime_error, "(car: need squote-List)");
    }

    return p_list->car();
}

/**
 * @brief (cdr '(list item1 item2 ...)) -> '(item2 item3 ...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 *
 * TODO FIXME 这个函数，需要类似eval_car改造。
 */
Object eval_cdr(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args, obj);
    if (!p_list) {
        SSS_POSTION_THROW(std::runtime_error, "(cdr: need squote-List)");
    }
    return p_list->cdr();
}

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
        SSS_POSTION_THROW(std::runtime_error,
                          "(car-nth: 1st argument must be an Integar)");
    }
    Object obj2;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args.tail[0], obj2);
    if (!p_list) {
        SSS_POSTION_THROW(std::runtime_error,
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

/**
 * @brief
 *      (cdr-nth index '(list)) -> list[index]
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
        SSS_POSTION_THROW(std::runtime_error,
                          "(car-nth: 1st argument must be an Integar)");
    }
    Object obj2;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args.tail[0], obj2);
    if (!p_list) {
        SSS_POSTION_THROW(std::runtime_error,
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
        SSS_POSTION_THROW(std::runtime_error, "(cons: need squote-List as the 2nd argument)");
    }
    varlisp::List ret = varlisp::List::makeSQuoteList();
    ret.append(boost::apply_visitor(eval_visitor(env), args.head));
    ret.tail[0].tail.push_back(p_list->tail[0]);
    return ret;
}

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
        SSS_POSTION_THROW(std::runtime_error, "(length: need s-List as the 1st argument)");
    }
    return int(p_list->length() - 1);
}

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
        SSS_POSTION_THROW(std::runtime_error, "(append: need s-List as the 1st argument)");
    }
    Object obj2;
    const varlisp::List * p_list2 = getFirstListPtrFromArg(env, args.tail[0], obj2);
    if (!p_list2) {
        SSS_POSTION_THROW(std::runtime_error, "(append: need s-List as the 2nd argument)");
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

}  // namespace varlisp
