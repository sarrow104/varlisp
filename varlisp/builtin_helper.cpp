#include "eval_visitor.hpp"
#include "object.hpp"

namespace varlisp {

bool is_literal_list(const varlisp::List& l)
{
    bool is_literal = false;
    if (l.length()) {
        const varlisp::symbol* p_symbol =
            boost::get<const varlisp::symbol>(&l.head);
        if (p_symbol && *p_symbol == varlisp::symbol("list")) is_literal = true;
    }
    return is_literal;
}

const varlisp::List* getFirstListPtrFromArg(varlisp::Environment& env,
                                            const varlisp::List& args,
                                            Object& tmp)
{
    // NOTE FIXME List 的第一个元素是symbol 的list!
    const varlisp::List* _list = boost::get<varlisp::List>(&(args.head));
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
}  // namespace varlisp
