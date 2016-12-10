#include <limits>
#include <vector>

#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "cast2bool_visitor.hpp"
#include "detail/buitin_info_t.hpp"
#include "detail/list_iterator.hpp"
#include "detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("map", 2, -1, eval_map,
               "; map函数接受一个函数和N个列表，该函数接受N个参数；\n"
               "; 返回一个列表。返回列表的每个元素都是使用输入的函数\n"
               "; 对N个类别中的每个元素处理的结果\n"

               "(map func list-1 list-2 ... list-n) ->\n"
               "\t'(func(l1[1] l2[1] ... ln[1])\n"
               "\t  func(l1[2] l2[2] ... ln[2])\n"
               "\t  ...\n"
               "\t  func(l1[n] l2[n] ... ln[n]))");

/**
 * @brief (map func list-1 list-2 ... list-n)
 *      -> '(func(l1[1] l2[1] ... ln[1]) func(l1[2] l2[2] ... ln[2]) ... func(l1[n] l2[n] ... ln[n]))
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_map(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "map";
    // NOTE 第一个参数是call-able；并且需要n个参数
    // 后续有n个s-list；并且，每个s-list的长度相同；
    // 需要注意的是，部分內建函数，我允许无限多个参数……
    // 部分list，元素个数不足怎么办？
    // 有如下选择，
    // 1. 是用最后一个元素重复使用；
    // 2. 传入nil或者'();
    // 3. 就此停止
    //
    // 感觉2不是很合适。因为用户可能显示传入nil或者'()作为参数——这样就无法区分了。
    // 重用最后一个元素也不会很合用；唯一合理的使用情形是，某list，只有一个元素。
    // 那么重用就显得合理。不然，不足的部分，你是选择重用第一个，还是最后一个元素呢？
    // 如何判断，是否是call-able 的Object？
    // 1. builtin
    // 2. lambda

    int arg_length = args.size() - 1;
    COLOG_DEBUG(SSS_VALUE_MSG(arg_length));
    std::vector<Object>      tmp_obj_vec;
    tmp_obj_vec.resize(arg_length);
    std::vector<const List*> p_arg_list_vec;
    p_arg_list_vec.resize(arg_length);

    int min_items_count = std::numeric_limits<int>::max();
    const List arg_list = args.tail();
    auto arg_list_it = arg_list.begin();
    for (int i = 0; i < arg_length; ++i, ++arg_list_it)
    {
        p_arg_list_vec[i] = varlisp::getQuotedList(env, *arg_list_it, tmp_obj_vec[i]);
        if (!p_arg_list_vec[i]) {
            SSS_POSITION_THROW(std::runtime_error,
                              "(", funcName, ": the other arguments must be s-list; "
                              " but", *arg_list_it, ")");
        }
        min_items_count = std::min(int(p_arg_list_vec[i]->length()), min_items_count);
    }

    COLOG_DEBUG(SSS_VALUE_MSG(min_items_count));
    varlisp::List ret = varlisp::List::makeSQuoteList();
    auto ret_it = detail::list_back_inserter<Object>(ret);
    const Object& callable = detail::car(args);
    for (int i = 0; i < min_items_count; ++i) {
        COLOG_DEBUG("loop ", i, "begin");
        varlisp::List expr {callable};
        auto back_it = detail::list_back_inserter<Object>(expr);

        for (int j = 0; j < arg_length; ++j) {
            COLOG_DEBUG(SSS_VALUE_MSG(i), ',', SSS_VALUE_MSG(j), ',', p_arg_list_vec[j]->nth(i));
            *back_it++ = p_arg_list_vec[j]->nth(i);
        }
        COLOG_DEBUG(expr);
        *ret_it++ = expr.eval(env);
        COLOG_DEBUG(ret);
        COLOG_DEBUG("loop ", i, "end");
    }

    return ret;
}

REGIST_BUILTIN("reduce", 2, 2, eval_reduce,
               "; reduce让一个指定的函数(function)作用于列表的第一个\n"
               "; 元素和第二个元素,然后再作用于上步得到的结果和第三个\n"
               "; 元素，直到处理完列表中所有元素。\n"

               "(reduce func list) ->\n"
               "\tfunc(func(func(l[1] l[2]) l[3]) ... l[n-1]) l[n])");

/**
 * @brief
 *      (reduce func list) 
 *          -> func(func(func(l[1] l[2]) l[3]) ... l[n-1]) l[n])
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_reduce(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "reduce";
    // NOTE 第一个参数是call-able；并且需要两个参数
    // 第二个参数，是不少于2个元素的s-list
    const Object& callable = detail::car(args);
    Object tmp;
    const List * p_arg_list = varlisp::getQuotedList(env, detail::cadr(args), tmp);
    if (!p_arg_list) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need a s-list at 2nd arguments)");
    }
    if (p_arg_list->length() < 2) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": the s-list must have at least two items)");
    }

    Object first_arg = p_arg_list->head();
    auto args_list = p_arg_list->tail();
    for (auto arg_it = args_list.begin(); arg_it != args_list.end(); ++arg_it) {
        Object second_arg = *arg_it;
        varlisp::List expr = varlisp::List( {callable, first_arg, second_arg});
        first_arg = expr.eval(env);
    }

    return first_arg;
}

REGIST_BUILTIN("filter", 2, 2, eval_filter,
               "; filter 根据func作用到每个元素的返回结果是否为#t；\n"
               "; 决定是否在返回的列表中，包含该元素\n"

               "(filter func list) ->\n"
               "\t(sigma list[i] where (func list[i]) == #t)");

/**
 * @brief (filter func list)
 *           -> (sigma list[i] where (func list[i]) == #t)
 *
 * @param env
 * @param args
 *
 * @return 
 */
Object eval_filter(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "filter";
    // NOTE 第一个参数是只接受一个参数的call-able；
    // 第二个参数，是一个s-list，元素个数不定；
    const Object& callable = detail::car(args);
    Object tmp;
    const List * p_arg_list = varlisp::getQuotedList(env, detail::cadr(args), tmp);
    if (!p_arg_list) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need a s-list at 2nd arguments)");
    }

    varlisp::List ret = varlisp::List::makeSQuoteList();
    auto ret_it = detail::list_back_inserter<Object>(ret);
    for (auto it = p_arg_list->begin(); it != p_arg_list->end(); ++it) {
        varlisp::List expr = varlisp::List({callable, *it});
        Object value = expr.eval(env);

        if (varlisp::is_true(env, value)) {
            *ret_it++ = *it;
        }
    }
    return ret;
}

REGIST_BUILTIN("pipe-run", 2, -1, eval_pipe_run,
               "; pipe-run 函数嵌套调用，有时候会很繁琐；\n"
               "; 对于前面一个调用的结果，可以用作后面一个调用的参数的情况，\n"
               "; 可以用管道式操作来简化；\n"

               "(pipe-run arguments func1 func2 ...) ->\n"
               "\t(funcN(...(func2(func1(argument))...))");

Object eval_pipe_run(varlisp::Environment &env, const varlisp::List &args)
{
    // const char * funcName = "pipe-run";
    // NOTE 第一个参数是call-able；并且需要两个参数
    // 第二个参数，是不少于2个元素的s-list
    Object tmp;
    Object argument = varlisp::getAtomicValue(env, detail::car(args), tmp);

    const varlisp::List func_list = args.tail();

    for (auto func_it = func_list.begin(); func_it != func_list.end(); ++func_it) {
        varlisp::List expr = varlisp::List({*func_it, argument});
        argument = expr.eval(env);
    }

    return argument;
}

REGIST_BUILTIN("is-all", 2, 2, eval_is_all,
               "; is-all 用第一个参数作为函数，测试列表中，是否每个元素都符合要求\n"
               "; 都符合，返回#t; 否则返回#f\n"
               "(is-all test-func [v1...]) -> boolean");

Object eval_is_all(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "is-all";
    // NOTE 第一个参数是call-able；并且需要两个参数
    // 第二个参数，是不少于1个元素的s-list
    const Object& callable = detail::car(args);
    Object tmp;
    const List * p_arg_list = varlisp::getQuotedList(env, detail::cadr(args), tmp);
    if (!p_arg_list || p_arg_list->empty()) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need a none-empty s-list at 2nd arguments; but ",
                          detail::cadr(args), ")");
    }

    bool is_all = true;
    for (auto it = p_arg_list->begin(); it != p_arg_list->end(); ++it) {
        varlisp::List expr = varlisp::List( {callable, *it});

        Object res = expr.eval(env);
        if (!boost::apply_visitor(cast2bool_visitor(env), res)) {
            is_all = false;
            break;
        }
    }

    return is_all;
}

REGIST_BUILTIN("is-any", 2, 2, eval_is_any,
               "; is-any 用第一个参数作为函数，测试列表中，是否存在某个元素符合要求\n"
               "; 如果找到一个符合的，立即返回#t; 否则返回#f\n"
               "(is-any test-func [v1...]) -> boolean");

Object eval_is_any(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "is-all";
    // NOTE 第一个参数是call-able；并且需要两个参数
    // 第二个参数，是不少于1个元素的s-list
    const Object& callable = detail::car(args);
    Object tmp;
    const List * p_arg_list = varlisp::getQuotedList(env, detail::cadr(args), tmp);
    if (!p_arg_list || p_arg_list->empty()) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need a none-empty s-list at 2nd arguments; but ",
                          detail::cadr(args), ")");
    }

    bool is_any = false;
    for (auto it = p_arg_list->begin(); it != p_arg_list->end(); ++it) {
        varlisp::List expr = varlisp::List( {callable, *it});

        Object res = expr.eval(env);
        if (boost::apply_visitor(cast2bool_visitor(env), res)) {
            is_any = true;
            break;
        }
    }

    return is_any;
}
} // namespace varlisp
