#include "builtin_helper.hpp"
#include "object.hpp"
#include "print_visitor.hpp"

#include "arithmetic_cast_visitor.hpp"
#include "arithmetic_t.hpp"
#include "builtin_helper.hpp"

#include <sss/spliter.hpp>

namespace varlisp {
/**
 * @brief 拆分字符串
 *      (split "string to split") -> '("part1","part2", ...)
 *      (split "string to split" "seq-str") -> '("part1","part2", ...)
 *
 * @param[in] env
 * @param[in] args 支持两个，参数，分别待切分字符串，和分割字符串；
 *
 * @return 分割之后的列表；
 *
 * TODO 支持正则表达式，确定sep!
 * 需要三个参数；
 * 其中第三个参数是表示正则的的symbol
 */
Object eval_split(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "split";
    Object content;
    const string_t *p_content =
        getTypedValue<string_t>(env, args.head, content);

    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requies string as 1st argument)");
    }

    std::string sep(1, ' ');
    if (args.length() == 2) {
        Object sep_obj;
        const string_t *p_sep =
            getTypedValue<string_t>(env, args.next()->head, sep_obj);
        if (!p_sep) {
            SSS_POSITION_THROW(std::runtime_error,
                              "(", funcName, ": requires seq string as 2nd argument)");
        }
        sep = p_sep->to_string();
    }
    varlisp::List ret = varlisp::List::makeSQuoteList();
    if (sep.length() == 1) {
        sss::ViewSpliter<char> sp(*p_content, sep[0]);
        List *p_list = &ret;
        sss::string_view stem;
        while (sp.fetch_next(stem)) {
            p_list = p_list->next_slot();
            p_list->head = p_content->substr(stem);
        }
    }
    else {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": sep.length() >= 2, not support yet!)");
    }
    return Object(ret);
}

/**
 * @brief join string list
 *      (join '("s1" "s2" ...)) -> "joined-text"
 *      (join '("s1" "s2" ...) "seq") -> "joined-text"
 *
 * @param[in] env
 * @param[in] args 第一个参数，必须是一个(list)；或者symbol
 *
 * @return
 */
Object eval_join(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "join";
    Object obj;
    const List *p_list = getFirstListPtrFromArg(env, args, obj);

    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": 1st must a list!)");
    }

    std::string sep;
    if (args.length() == 2) {
        Object sep_obj;
        const string_t *p_sep = getTypedValue<string_t>(env, args.tail[0].head, sep_obj);
        if (!p_sep) {
            SSS_POSITION_THROW(std::runtime_error,
                              "(", funcName, ": 2nd sep must be a string)");
        }
        sep = p_sep->to_string();
    }

    std::ostringstream oss;

    bool is_first = true;
    p_list = p_list->next();
    while (p_list) {
        Object obj;
        const string_t *p_stem = getTypedValue<string_t>(env, p_list->head, obj);
        if (!p_stem) {
            break;
        }
        if (is_first) {
            is_first = false;
        }
        else {
            oss << sep;
        }
        oss << *p_stem;
        p_list = p_list->next();
    }

    return Object(string_t(std::move(oss.str())));
}

/**
 * @brief
 *    (substr "target-string" offset)
 *    (substr "target-string" offset length)
 *      -> sub-str
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_substr(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "substr";
    const List *p_arg = &args;
    Object content;
    const string_t *p_content =
        getTypedValue<string_t>(env, p_arg->head, content);
    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": need string as 1st argument)");
    }
    p_arg = p_arg->next();

    Object offset;
    const Object &offset_ref = getAtomicValue(env, p_arg->head, offset);

    arithmetic_t offset_number =
        boost::apply_visitor(arithmetic_cast_visitor(env), (offset_ref));
    if (!offset_number.which()) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": need int as 2nd argument)");
    }
    int offset_int = arithmetic2int(offset_number);

    if (offset_int < 0) {
        offset_int = 0;
    }
    if (offset_int > int(p_content->length())) {
        offset_int = p_content->length();
    }

    int length = -1;
    if (args.length() == 3) {
        p_arg = p_arg->next();
        Object length_obj;
        const Object &length_obj_ref =
            getAtomicValue(env, p_arg->head, length_obj);
        arithmetic_t arithmetic_length = boost::apply_visitor(
            arithmetic_cast_visitor(env), (length_obj_ref));
        if (!arithmetic_length.which()) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                              ": need int as 3rd argument)");
        }
        length = arithmetic2int(arithmetic_length);
    }

    if (length < 0) {
        return p_content->substr(offset_int);
    }
    else {
        return p_content->substr(offset_int, length);
    }
}

/**
 * @brief
 *    (strlen "target-string")
 *      -> length
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_strlen(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "strlen";
    Object obj;
    const string_t *p_str = getTypedValue<string_t>(env, args.head, obj);
    if (!p_str) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": need an s-List as the 1st argument)");
    }
    return int(p_str->length());
}
}  // namespace varlisp
