#include "object.hpp"
#include "builtin_helper.hpp"

#include "environment.hpp"

namespace varlisp {
/**
 * @brief (defer (expr)) -> result-of-expr | ignore
 *   在上层语句完成之前，执行expr；
 *   如果实现起来困难，可以记为：
 *   (defer (defer-expr) expr1 expr2 ...)
 *   即，顺序执行后续指令；如果出现问题，从中间执行序列跳出的时候，
 *   仍然执行defer-expr。
 *   像这样，就容易写了。但意味着，需要额外添加括号。
 *   另外有一个问题是，如果我在交互式解析器，使用这个工具，又如何
 *   处理？
 *   难道说，这个交互式解析器，需要挂载这个defer么？比如，等到我
 *   输入(quit)的时候，会逆序执行这个defer序列？
 *   也就是说，这个defer，需要注册到 Environment 下。
 *   当这个Environment发生析构的时候，会依次调用。
 *   并且，就算是异常发生，析构也会照样完成……
 *   当然，得防止defer语句本身抛出异常。
 *   不过，问题来了，defer语句如果真的出了问题，我是终止解释器，
 *   还是仅放弃当前defer语句执行呢？
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_defer(varlisp::Environment& env, const varlisp::List& args)
{
    env.defer_task_push(args.head);
    return varlisp::Nill{};
}
} // namespace varlisp
