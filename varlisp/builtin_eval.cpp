#include "object.hpp"
#include "eval_visitor.hpp"

namespace varlisp {
Object eval_eval(varlisp::Environment& env, const varlisp::List& args)
{
    int arg_length = args.length();
    if (arg_length == 2) {
        SSS_POSTION_THROW(std::runtime_error,
                          "eval only support one arguments now!");
        // TODO
        // 至于环境变量，可以根据传入进来的symbol，然后通过全局函数，获取环
        // 境变量的引用，然后应用到这里。
    }
    // 需要先对参数eval一次，然后再eval一次！
    // 为什么需要两次？
    // 因为，既然是通过eval调用进来的，那么，这个eval，根据lisp的语法，必须
    // 先解析()内部，然后作为外部的参数再次解析。
    //
    // 所以，这两次eval动作，分别是原有的用户动作，和这个eval关键字自己引发的动作；
    varlisp::Object first_res = boost::apply_visitor(eval_visitor(env), args.head);
    return boost::apply_visitor(eval_visitor(env), first_res);
}

} // namespace varlisp
