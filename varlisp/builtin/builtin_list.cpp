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

    // NOTE 不能直接append！必须要求值！
    ret.append_list(env, *p_list1);
    ret.append_list(env, *p_list2);

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

namespace detail {
// TODO start nil -> 0
// stop nil -> -1
// step nil -> 1
std::tuple<int64_t, int64_t, int64_t> range_require_impl(varlisp::Environment& env, const varlisp::List& args, const char *funcName, int index_offset)
{
    std::array<Object, 3> objs;
    std::tuple<int32_t, int32_t, int32_t> range {0, 0, 1};

    std::get<0>(range) =
        *varlisp::requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0 + index_offset, DEBUG_INFO);

    std::get<1>(range) =
        *varlisp::requireTypedValue<int64_t>(env, args.nth(1), objs[1], funcName, 1 + index_offset, DEBUG_INFO);

    if (args.size() >= 3) {
        std::get<2>(range) =
            *varlisp::requireTypedValue<int64_t>(env, args.nth(2), objs[2], funcName, 2 + index_offset, DEBUG_INFO);

        if (0 == std::get<2>(range)) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName, ", step == zero!)");
        }
    }

    return range;
}

int64_t index_regular(int64_t index, uint32_t length)
{
    if (index < 0) {
        index += int64_t(length);
    }
    if (index > int64_t(length)) {
        index = length;
    }
    if (index < 0) {
        index = 0;
    }
    return index;
}

} // namespace detail

// NOTE python的方括号，切片是按照闭合、开放区间返回的。
// 另外，如果step为负数，就表示返回一个逆向list；
// 另外，范围不对的话，会返回一个空list。
// 然后，正数end，可以大于等于当前list的长度。当然，返回的list，并不会超过原有的list长！
// 我这里，如何设定？用半开半闭，还是双闭合？
// 还有，负数的end，又表示什么样的范围呢？
//
// 另外，如果为了节省内存的话，可以考虑内嵌一个index列表，来重复引用list。
// 即，varlisp::List，内部再保存一个optinal<std::vector<int>>的成员，用来保存，
// 可能的切片结构；
REGIST_BUILTIN("slice", 3, 4, eval_slice,
               "; slice 切片截取\n"
               "; begin,end 都是表示位置的点；如果begin为nil，表示0；"
               "; end 如果为nil，表示取最后一位\n"
               "; step 默认是1；\n"
               "(slice '(list) begin end)\n"
               "(slice '(list) begin end step)");

Object eval_slice(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "slice";
    std::array<Object, 1> objs;
    const auto * p_list = getQuotedList(env, args.nth(0), objs[0]);
    varlisp::requireOnFaild<varlisp::QuoteList>(p_list, funcName, 0, DEBUG_INFO);

    varlisp::List ret = varlisp::List();
    int64_t start = 0, stop = -1, step = 1;
    std::tie(start, stop, step) = detail::range_require_impl(env, args.tail(), funcName, 1);
    start = detail::index_regular(start, p_list->size());
    stop = detail::index_regular(stop, p_list->size());
    for (auto i = start; i >= 0 && i < int64_t(p_list->size()) && i != stop; i += step) {
        ret.append(p_list->nth(i));
    }
    return varlisp::List::makeSQuoteObj(ret);
}

REGIST_BUILTIN("range", 2, 3, eval_range,
               "; range 返回一个序列\n"
               "; 语义，同for\n"
               "; 实现逻辑，相当于定义一个列表，for内部，就不断往这个列表添加东西\n"
               "; 或者，干脆用for来实现\n"
               "; 好像不行；因为我要求range能接受可变参数。而可变参数，必须由解释器来实现……"
               "(range begin end)\n"
               "(range begin end step)");

Object eval_range(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "range";

    varlisp::List ret = varlisp::List();
    int64_t start = 0, stop = -1, step = 1;
    std::tie(start, stop, step) = detail::range_require_impl(env, args, funcName, 0);
    auto offset = stop - start;
    auto times = offset / step;
    if (times < 0) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName, ", wrong range:", start, stop, step, ")");
    }
    for (auto i = 0; i <= times && start + step * i != stop; ++i) {
        ret.append(start + step * i);
    }
    return varlisp::List::makeSQuoteObj(ret);
}

// TODO reverse reverse也可以用for来实现……
// (define (reverse l)
//  (letn ((ret-l []))
//   (format 1 "{}\n" l)
//   (for (item l)
//    (format 1 "{}\n" item)
//    (setq ret-l (cons item ret-l))
//   )
//  )
// )
// inter-set 交集
// outter-set 并集

}  // namespace varlisp
