#include "../object.hpp"

#include "../builtin_helper.hpp"
#include "../builtin_helper.hpp"
#include "../strict_equal_visitor.hpp"
#include "../strict_less_visitor.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"

namespace varlisp {
// 对于drracket来说，比较运算符，它会直接要求转换到real域；
// 如果失败，则抛出异常；
// 即，lambda函式，与1.2实数的比较大小，会出错，抛出异常：
// > (if (> fib 1) 1 2)
// >: contract violation
//   expected: real?
//   given: #<procedure:fib>
//   argument position: 1st
//   other arguments...:
//    1
// 其中：
// > (define fib (lambda (x) (if (> x 2) (+ (fib (- x 1)) (fib (- x 2))) 1)))

REGIST_BUILTIN("=", 2, 2, eval_eq, "(= arg1 arg2) -> boolean");

/**
 * @brief (= obj1 obj2) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_eq(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, detail::car(args), obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, detail::cadr(args), obj2);

    return {boost::apply_visitor(strict_equal_visitor(env), obj1_ref, obj2_ref)};
}

REGIST_BUILTIN("!=", 2, 2, eval_not_eq, "(!= arg1 arg2) -> boolean");

/**
 * @brief
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_not_eq(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, detail::car(args), obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, detail::cadr(args), obj2);

    return !boost::apply_visitor(strict_equal_visitor(env), obj1_ref, obj2_ref);
}

REGIST_BUILTIN(">", 2, 2, eval_gt, "(> arg1 arg2) -> boolean");

/**
 * @brief (> obj1 obj2) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_gt(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, detail::car(args), obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, detail::cadr(args), obj2);
    return !boost::apply_visitor(strict_equal_visitor(env), obj2_ref, obj1_ref) &&
                  boost::apply_visitor(strict_less_visitor(env), obj2_ref, obj1_ref);
}

REGIST_BUILTIN("<", 2, 2, eval_lt, "(< arg1 arg2) -> boolean");

/**
 * @brief (< obj1 obj2) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_lt(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, detail::car(args), obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, detail::cadr(args), obj2);
    return boost::apply_visitor(strict_less_visitor(env), obj1_ref, obj2_ref);
}

REGIST_BUILTIN(">=", 2, 2, eval_ge, "(>= arg1 arg2) -> boolean");

/**
 * @brief (>= obj1 obj2) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_ge(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, detail::car(args), obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, detail::cadr(args), obj2);
    return !boost::apply_visitor(strict_less_visitor(env), obj1_ref, obj2_ref);
}

REGIST_BUILTIN("<=", 2, 2, eval_le, "(<= arg1 arg2) -> boolean");

/**
 * @brief (<= obj1 obj2) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_le(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const Object& obj1_ref = varlisp::getAtomicValue(env, detail::car(args), obj1);
    Object obj2;
    const Object& obj2_ref = varlisp::getAtomicValue(env, detail::cadr(args), obj2);
    return boost::apply_visitor(strict_equal_visitor(env), obj2_ref, obj1_ref) ||
                  boost::apply_visitor(strict_less_visitor(env), obj1_ref, obj2_ref);
}

REGIST_BUILTIN("not", 1, 1, eval_not, "(not expr) -> boolean");

/**
 * @brief (not expr) -> !#t | !#f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_not(varlisp::Environment& env, const varlisp::List& args)
{
    return !varlisp::is_true(env, detail::car(args));
}

REGIST_BUILTIN("equal", 2, 2, eval_equal,
               "(equal '(list1) '(list2)) -> #t | #f");

/**
 * @brief
 *    (equal '(list1) '(list2)) -> #t | #f
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_equal(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const varlisp::List * p_list1 = varlisp::getQuotedList(env, detail::car(args), obj1);
    if (p_list1 == nullptr) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(equal: 1st argument must be an s-list)");
    }
    Object obj2;
    const varlisp::List * p_list2 = varlisp::getQuotedList(env, detail::cadr(args), obj2);
    if (p_list2 == nullptr) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(equal: 1st argument must be an s-list)");
    }
    bool is_equal = (p_list1->length() == p_list2->length());
    if (is_equal) {
        auto it1 = p_list1->begin();
        auto it2 = p_list2->begin();
        while (it1 != p_list1->end() && it2 != p_list2->end()) {
            if (!boost::apply_visitor(strict_equal_visitor(env),
                                    *it1, *it2)) {
                is_equal = false;
                break;
            }
            ++it1;
            ++it2;
        }
    }
    return is_equal;
}

}  // namespace varlisp
