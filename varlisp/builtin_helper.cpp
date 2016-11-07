#include "object.hpp"
#include "eval_visitor.hpp"

namespace varlisp {
const varlisp::List * getFirstListPtrFromArg(varlisp::Environment& env, const varlisp::List& args, Object& tmp)
{
    // NOTE List 的第一个元素是symbol 的list!
    const varlisp::List * _list = boost::get<varlisp::List>(&(args.head));
    if (!_list) {
        // list变量，会少一个`list`的symbol，因此处理起来不同；
        tmp = boost::apply_visitor(eval_visitor(env), args.head);
        _list = boost::get<varlisp::List>(&tmp);
    }
    else {
        // descard `list` symbol
        _list = _list->next();
    }
    return _list;
}
} // namespace varlisp
