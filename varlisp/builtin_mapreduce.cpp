#include <limits>
#include <vector>

#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"

namespace varlisp {

// 如何判断，是否是call-able 的Object？
// 1. builtin
// 2. lambda
//
// 还有允许的参数个数……

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

    int arg_length = args.next()->length();
    COLOG_DEBUG(SSS_VALUE_MSG(arg_length));
    std::vector<Object>      tmp_obj_vec;
    tmp_obj_vec.resize(arg_length);
    std::vector<const List*> p_arg_list_vec;
    p_arg_list_vec.resize(arg_length);

    int min_items_count = std::numeric_limits<int>::max();
    const List * p_arg_list = args.next();
    for (int i = 0; i < arg_length; ++i, p_arg_list = p_arg_list->next())
    {
        p_arg_list_vec[i] = varlisp::getFirstListPtrFromArg(env, *p_arg_list, tmp_obj_vec[i]);
        if (!p_arg_list_vec[i]) {
            SSS_POSITION_THROW(std::runtime_error,
                              "(reduce: the other arguments must be s-list)");
        }
        p_arg_list_vec[i] = p_arg_list_vec[i]->next(); // descard "list"
        min_items_count = std::min(int(p_arg_list_vec[i]->length()), min_items_count);
    }

    COLOG_DEBUG(SSS_VALUE_MSG(min_items_count));
    varlisp::List ret = varlisp::List::makeSQuoteList();
    varlisp::List * p_ret_list = &ret;
    const Object& callable = args.head;
    for (int i = 0; i < min_items_count; ++i) {
        COLOG_DEBUG("loop ", i, "begin");
        varlisp::List expr {callable};
        varlisp::List * p_expr = &expr;

        for (int j = 0; j < arg_length; ++j) {
            COLOG_DEBUG(SSS_VALUE_MSG(i), ',', SSS_VALUE_MSG(j), ',', p_arg_list_vec[j]->head);
            p_expr = p_expr->next_slot();
            p_expr->head = p_arg_list_vec[j]->head;
            p_arg_list_vec[j] = p_arg_list_vec[j]->next();
        }
        COLOG_DEBUG(expr);
        p_ret_list = p_ret_list->next_slot();
        p_ret_list->head = expr.eval(env);
        COLOG_DEBUG(ret);
        COLOG_DEBUG("loop ", i, "end");
    }

    return ret;
}

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
    // NOTE 第一个参数是call-able；并且需要两个参数
    // 第二个参数，是不少于2个元素的s-list
    const Object& callable = args.head;
    Object tmp;
    const List * p_arg_list = varlisp::getFirstListPtrFromArg(env, *args.next(), tmp);
    if (!p_arg_list) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(reduce: need a s-list at 2nd arguments)");
    }
    if (p_arg_list->length() < 3) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(reduce: the s-list must have at least two items)");
    }

    p_arg_list = p_arg_list->next();
    Object first_arg = p_arg_list->head;
    p_arg_list = p_arg_list->next();
    while (p_arg_list && p_arg_list->head.which()) {
        Object second_arg = p_arg_list->head;
        varlisp::List expr {callable};
        expr.append(first_arg);
        expr.append(second_arg);
        first_arg = expr.eval(env);
        p_arg_list = p_arg_list->next();
    }

    return first_arg;
}

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
    // NOTE 第一个参数是只接受一个参数的call-able；
    // 第二个参数，是一个s-list，元素个数不定；
    const Object& callable = args.head;
    Object tmp;
    const List * p_arg_list = varlisp::getFirstListPtrFromArg(env, *args.next(), tmp);
    if (!p_arg_list) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(reduce: need a s-list at 2nd arguments)");
    }

    p_arg_list = p_arg_list->next();

    varlisp::List ret = varlisp::List::makeSQuoteList();
    varlisp::List * p_ret_list = &ret;
    int arg_cnt = p_arg_list->length();
    for (int i = 0; i < arg_cnt; ++i, p_arg_list = p_arg_list->next()) {
        varlisp::List expr {callable};

        expr.append(p_arg_list->head);
        Object value = expr.eval(env);

        if (varlisp::is_true(env, value)) {
            p_ret_list = p_ret_list->next_slot();
            p_ret_list->head = p_arg_list->head;
        }
    }
    return ret;
}

} // namespace varlisp
