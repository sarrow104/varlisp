#include <iterator>

#include <sss/util/utf8.hpp>
#include <sss/spliter.hpp>
#include <sss/colorlog.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "print_visitor.hpp"
#include "arithmetic_cast_visitor.hpp"
#include "arithmetic_t.hpp"
#include "builtin_helper.hpp"

#include "detail/buitin_info_t.hpp"
#include "detail/list_iterator.hpp"

namespace varlisp {

REGIST_BUILTIN("split", 1,  2,  eval_split,
               "(split \"string to split\") -> '(\"part1\",\"part2\", ...)\n"
               "(split \"string to split\" \"seq-str\") -> '(\"part1\",\"part2\", ...)");

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
        auto ret_it = detail::list_back_inserter<Object>(ret);
        sss::string_view stem;
        while (sp.fetch_next(stem)) {
            *ret_it++ = p_content->substr(stem);
        }
    }
    else {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": sep.length() >= 2, not support yet!)");
    }
    return Object(ret);
}

REGIST_BUILTIN("join", 1, 2, eval_join,
               "(join '(\"s1\" \"s2\" ...)) -> \"joined-text\"\n"
               "(join '(\"s1\" \"s2\" ...) \"seq\") -> \"joined-text\"");

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

REGIST_BUILTIN("substr", 2, 3, eval_substr,
               "(substr \"target-string\" offset)\n"
               "(substr \"target-string\" offset length) -> sub-str");

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
                          ": need int64_t as 2nd argument)");
    }
    int64_t offset_int = arithmetic2int(offset_number);

    if (offset_int < 0) {
        offset_int = 0;
    }
    if (offset_int > int64_t(p_content->length())) {
        offset_int = p_content->length();
    }

    int64_t length = -1;
    if (args.length() == 3) {
        p_arg = p_arg->next();
        Object length_obj;
        const Object &length_obj_ref =
            getAtomicValue(env, p_arg->head, length_obj);
        arithmetic_t arithmetic_length = boost::apply_visitor(
            arithmetic_cast_visitor(env), (length_obj_ref));
        if (!arithmetic_length.which()) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                              ": need int64_t as 3rd argument)");
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

REGIST_BUILTIN("strlen", 1, 1, eval_strlen,
               "(strlen \"target-string\") -> length");

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
                          ": need an string as the 1st argument)");
    }
    return int64_t(p_str->length());
}

REGIST_BUILTIN("split-char", 1, 1, eval_split_char,
               "(split-char \"target-string\") -> '(int64_t-char1 int64_t-char2 ...)");

// NOTE TODO 或许需要这样一个函数，给一个type列表，然后返回转换的结果；
// 可以转换的，这个列表对应的指针，就是非0；
// 错误信息呢？
// 最好传入1：函数名；当前参数位置；然后类型名直接显示吗？也不对；底层
// 类型名和展示给用户的类型也是不同的。

/**
 * @brief
 *    (split-char "target-string")
 *      -> '(int64_t-char1 int64_t-char2 ...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_split_char(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "split-char";
    Object obj;
    const varlisp::string_t *p_str =
        varlisp::getTypedValue<varlisp::string_t>(env, args.head, obj);
    if (!p_str) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": need an string as the 1st argument)");
    }
    varlisp::List ret = varlisp::List::makeSQuoteList();
    sss::util::utf8::dumpout2ucs(
        p_str->begin(), p_str->end(),
        detail::list_back_inserter<int64_t>(ret));
    return ret;
}

REGIST_BUILTIN("join-char",       1,  1,  eval_join_char,
               "(join-char '(int64_t-char1 int64_t-char2 ...)) -> \"string\"");

/**
 * @brief
 *    (join-char '(int-char1 int-char2 ...))
 *      -> "string"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_join_char(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "join-char";
    Object obj;
    const varlisp::List * p_list = varlisp::getFirstListPtrFromArg(env, args, obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": need quote list as the 1st argument)");
    }
    p_list = p_list->next();
    std::string ret;
    sss::util::utf8::dumpout2utf8(detail::list_const_iterator_t<int64_t>(p_list),
                                  detail::list_const_iterator_t<int64_t>(nullptr),
                                  std::back_inserter(ret));
    return varlisp::string_t{std::move(ret)};
}

REGIST_BUILTIN("split-byte",       1,  1,  eval_split_byte,
               "(split-byte \"target-string\") -> '(int64_t-byte1 int64_t-byte2 ...)");

Object eval_split_byte(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "split-byte";
    Object obj;
    const varlisp::string_t *p_str =
        varlisp::getTypedValue<varlisp::string_t>(env, args.head, obj);
    if (!p_str) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": need an string as the 1st argument)");
    }
    varlisp::List ret = varlisp::List::makeSQuoteList();

    // NOTE  谨防 0x80 0xFF 字符可能引起问题
    auto back_it = detail::list_back_inserter<int64_t>(ret);
    for (auto it = p_str->begin(); it != p_str->end(); ++it) {
        *back_it++ = int64_t(uint8_t(*it));
    }
    return ret;
}

REGIST_BUILTIN("join-byte",       1,  1,  eval_join_byte,
               "(join-byte '(int64_t-byte1 int64_t-byte2 ...)) -> \"string\"");

Object eval_join_byte(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "join-byte";
    Object obj;
    const varlisp::List * p_list = varlisp::getFirstListPtrFromArg(env, args, obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": need quote list as the 1st argument)");
    }
    p_list = p_list->next();
    std::string ret;
    // FIXME 谨防 0x80 0xFF 字符可能引起问题
    for (auto it = detail::list_const_iterator_t<int64_t>(p_list); it; ++it) {
        ret.push_back(char(*it & 0xFF));
    }
    return varlisp::string_t{std::move(ret)};
}

}  // namespace varlisp
