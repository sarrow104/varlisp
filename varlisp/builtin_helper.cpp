#include "object.hpp"
#include "eval_visitor.hpp"

namespace varlisp {
const varlisp::List * getFirstListPtrFromArg(varlisp::Environment& env, const varlisp::List& args, Object& tmp)
{
    // NOTE List �ĵ�һ��Ԫ����symbol ��list!
    const varlisp::List * _list = boost::get<varlisp::List>(&(args.head));
    if (!_list) {
        // list����������һ��`list`��symbol����˴���������ͬ��
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
