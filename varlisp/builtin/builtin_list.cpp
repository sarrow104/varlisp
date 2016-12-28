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
        requireTypedValue<varlisp::List>(env, args.nth(0), obj, funcName, 0, DEBUG_INFO);
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
    const char * funcName = "car-nth";
    std::array<Object, 2> objs;
    const int64_t * p_nth =
        requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    const varlisp::List* p_list = varlisp::getQuotedList(env, args.nth(1), objs[1]);
    varlisp::requireOnFaild<QuoteList>(p_list, funcName, 1, DEBUG_INFO);
    int index = *p_nth >= 0 ? *p_nth : p_list->length() + *p_nth;
    if (index < 0) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": 2nd argument too small");
    }
    return p_list->nth(index);
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
    const char * funcName = "cdr-nth";
    std::array<Object, 2> objs;
    const int64_t * p_nth =
        requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    const varlisp::List* p_list =
        requireTypedValue<List>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    int index = *p_nth >= 0 ? *p_nth : p_list->length() + *p_nth;
    if (index < 0) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": 2nd argument too small");
    }
    return p_list->cdr(index);
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
    const char * funcName = "cons";
    std::array<Object, 2> objs;
    const varlisp::List * p_list = varlisp::getQuotedList(env, args.nth(1), objs[1]);
    requireOnFaild<varlisp::QuoteList>(p_list, funcName, 1, DEBUG_INFO);

    const Object& headRef = varlisp::getAtomicValueUnquote(env, args.nth(0), objs[0]);
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
// FIXME double quote
// > (cons 1 '[2])
// (1 quote (2))

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
    const char * funcName = "append";
    std::array<Object, 2> objs;
    const varlisp::List * p_list1 = getQuotedList(env, args.nth(0), objs[0]);
    if (!p_list1) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": need s-List as the 1st argument)");
    }
    const varlisp::List * p_list2 = getQuotedList(env, args.nth(1), objs[1]);
    if (!p_list2) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": need s-List as the 2nd argument)");
    }
    varlisp::List ret = varlisp::List();

    ret.append_list(*p_list1);
    ret.append_list(*p_list2);

    return varlisp::List::makeSQuoteObj(ret);
}

namespace detail {

void append_flat(varlisp::List& out, const varlisp::List& ref)
{
    for (size_t i = 0; i < ref.length(); ++i) {
        if (const auto * p_inner = boost::get<varlisp::List>(&ref.nth(i))) {
            // NOTE 内部的quote，当做单一元素，也就不必检查到底是'(list...)还是
            // 'symbol
            if (p_inner->is_quoted()) {
                out.append(ref.nth(i));
            }
            else {
                detail::append_flat(out, *p_inner);
            }
        }
        else {
            out.append(ref.nth(i));
        }
    }
}
} // namespace detail

REGIST_BUILTIN("flat", 1, 1, eval_flat,
               "; flat 将quote列表，平坦化\n"
               "(flat '(list...)) -> '(list...)");

Object eval_flat(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "flat";
    std::array<Object, 1> objs;
    const varlisp::List * p_list = getQuotedList(env, args.nth(0), objs[0]);
    varlisp::requireOnFaild<varlisp::QuoteList>(p_list, funcName, 0, DEBUG_INFO);

    varlisp::List ret = varlisp::List();

    detail::append_flat(ret, *p_list);

    return varlisp::List::makeSQuoteObj(ret);
}

}  // namespace varlisp
