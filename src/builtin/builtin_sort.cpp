#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>

#include <algorithm>

#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../cast2bool_visitor.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/list_iterator.hpp"

namespace varlisp {

namespace detail {
varlisp::List sort_impl(varlisp::Environment& env, const varlisp::Object& callable,
                        const varlisp::List * p_list)
{
    auto arg_length = p_list->length();
    COLOG_DEBUG(SSS_VALUE_MSG(arg_length));
    std::vector<Object>      tmp_obj_vec;
    tmp_obj_vec.resize(arg_length);
    std::vector<const Object*> p_arg_list_vec;
    p_arg_list_vec.resize(arg_length);

    int i = 0;
    for (auto it = p_list->begin(); it != p_list->end(); ++it, ++i) {
        p_arg_list_vec[i] = &varlisp::getAtomicValue(env, *it, tmp_obj_vec[i]);
    }

    // NOTE >< 等比较函数，对于字符串无效……
    std::sort(p_arg_list_vec.begin(), p_arg_list_vec.end(),
              [&env,&callable](const Object* v1, const Object* v2)->bool{
                  varlisp::List expr {callable, *v1, *v2};
                  COLOG_DEBUG(callable, *v1, *v2);
                  Object ret = expr.eval(env);
                  return boost::apply_visitor(cast2bool_visitor(env), ret);
              });

    varlisp::List ret = varlisp::List::makeSQuoteList();
    auto back_it = detail::list_back_inserter<Object>(ret);

    for (size_t i = 0; i < arg_length; ++i) {
        *back_it++ = *p_arg_list_vec[i];
    }
    return ret;
}
} // namespace detail

REGIST_BUILTIN("sort", 2, 2, eval_sort,
               "(sort func '(list)) -> '(sorted-list)");

/**
 * @brief
 *      (sort func '(list)) -> '(sorted-list)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_sort(varlisp::Environment& env, const varlisp::List& args)
{
    // 流程，先创建一个vector<> tmpobjs; 然后创建一个vector<Object*>用来排序；
    // 这里，需要细心设计函数；
    // 需要注意的是，因为第一个参数是比较用函数。第二个参数是一个(list) —— 
    // 此时，list是不应该被求值的。就算某元素是一个lambda，也需要原样保留。
    // 就算是不合法的(expr)，也不能计算。
    // 就是说，需要"字面"比较！
    // 字面值还好说，难道变量也不行吗？
    // 我就想得到一个字面值的列表应该怎么做？必须利用cons！
    // 不然，只能得到一个由"符号"组成的(list).
    // 由于不能求值，这问题就麻烦了，我还不能方便地比较大小！
    //
    // 这个函数，难于编写的原因在于，我想让用户可以提供任意的比较函数，这些函数
    // ，应该是可以接受任何类型参数的。但是(list)的本质，又要求不能发生求值，就
    // 算某元素是(+ 1 2)，也只能被当做字面值，(list + 1 2)来处理，而不能求值。
    // 对于符号var，也必须看做是(quote var)，同样不能求值。
    // 就是说，对于字面值，我可以直接传入用户提供的比较函数；对于符号，包装为
    // (quote ?)；至于其他的 expr，就需要套一层list。
    //
    // 我还是先实现字面值吧！

    const char * funcName = "sort";
    const Object& callable = detail::car(args);

    Object listObj;
    const varlisp::List * p_list = varlisp::getQuotedList(env, detail::cadr(args), listObj);
    if (p_list == nullptr) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": second must be list; but",
                           detail::cadr(args), ")");
    }

    return detail::sort_impl(env, callable, p_list);
}

REGIST_BUILTIN("sort!", 2, 2, eval_sort_bar,
               "(sort! func '(list)) -> Nil ; sort in-place");

/**
 * @brief
 *      (sort! func '(list)) -> nil
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_sort_bar(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "sort!";
    const Object& callable = detail::car(args);

    Object symObj;
    const auto *p_sym = varlisp::getSymbol(env, detail::cadr(args), symObj);
    if (p_sym == nullptr) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": second must be symbol)");
    }
    Object * p_value = env.deep_find(p_sym->name());
    if (p_value == nullptr) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName, ": symbol, ",
                           *p_sym, " not exist!)");
    }

    Object listObj;
    const varlisp::List * p_list = varlisp::getQuotedList(env, *p_value, listObj);
    if (p_list == nullptr) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": second must be list); but ",
                           *p_value, ")");
    }

    varlisp::List ret = detail::sort_impl(env, callable, p_list);
    std::swap(*boost::get<varlisp::List>(p_value), ret);
    return varlisp::Nill{};
}

} // namespace varlisp
