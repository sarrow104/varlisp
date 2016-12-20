#include <array>
#include <iterator>

#include <sss/util/utf8.hpp>
#include <sss/spliter.hpp>
#include <sss/colorlog.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../print_visitor.hpp"
#include "../arithmetic_cast_visitor.hpp"
#include "../arithmetic_t.hpp"
#include "../builtin_helper.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/list_iterator.hpp"
#include "../detail/car.hpp"
#include "../raw_stream_visitor.hpp"

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
 */
Object eval_split(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "split";
    std::array<Object, 2> objs;
    const string_t *p_content =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::string sep(1, ' ');
    if (args.length() == 2) {
        const string_t *p_sep =
            requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);
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
    std::array<Object, 2> objs;
    const List *p_list = getQuotedList(env, detail::car(args), objs[0]);

    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": 1st must a list!)");
    }

    std::string sep;
    if (args.length() == 2) {
        const string_t *p_sep =
            requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);
        sep = p_sep->to_string();
    }

    std::ostringstream oss;

    bool is_first = true;
    for (auto it = p_list->begin(); it != p_list->end(); ++it) {
        Object obj;
        const string_t *p_stem = getTypedValue<string_t>(env, *it, obj);
        if (is_first) {
            is_first = false;
        }
        else {
            oss << sep;
        }
        
        if (!p_stem) {
            boost::apply_visitor(raw_stream_visitor(oss, env), *it);
        }
        else {
            oss << *p_stem;
        }
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
    std::array<Object, 3> objs;
    const string_t *p_content =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const Object &offset_ref = getAtomicValue(env, args.nth(1), objs[1]);

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
        const Object &length_obj_ref =
            getAtomicValue(env, args.nth(2), objs[2]);

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

REGIST_BUILTIN("ltrim", 1, 1, eval_ltrim,
               "(ltrim \"target-string\") -> \"left-trimed-string\"");

Object eval_ltrim(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "ltrim";
    std::array<Object, 1> objs;
    const string_t *p_str = 
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    sss::string_view sv = p_str->to_string_view();
    while(!sv.empty() && std::isspace(sv.front())) {
        sv.pop_front();
    }
    return p_str->substr(sv);
}

REGIST_BUILTIN("rtrim", 1, 1, eval_rtrim,
               "(rtrim \"target-string\") -> \"right-trimed-string\"");

Object eval_rtrim(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "rtrim";
    std::array<Object, 1> objs;
    const string_t *p_str =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    sss::string_view sv = p_str->to_string_view();
    while(!sv.empty() && std::isspace(sv.back())) {
        sv.pop_back();
    }
    return p_str->substr(sv);
}

REGIST_BUILTIN("trim", 1, 1, eval_trim,
               "(trim \"target-string\") -> \"bothsides-trimed-string\"");

Object eval_trim(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "trim";
    std::array<Object, 1> objs;
    const string_t *p_str = 
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    sss::string_view sv = p_str->to_string_view();
    while(!sv.empty() && std::isspace(sv.front())) {
        sv.pop_front();
    }
    while(!sv.empty() && std::isspace(sv.back())) {
        sv.pop_back();
    }
    return p_str->substr(sv);
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
    std::array<Object, 1> objs;
    const string_t *p_str = 
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
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
    std::array<Object, 1> objs;
    const string_t *p_str = 
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

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
    const varlisp::List * p_list = varlisp::getQuotedList(env, detail::car(args), obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": need quote list as the 1st argument)");
    }
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
    std::array<Object, 1> objs;
    const string_t *p_str = 
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

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
    const varlisp::List * p_list = varlisp::getQuotedList(env, detail::car(args), obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": need quote list as the 1st argument)");
    }
    std::string ret;
    // FIXME 谨防 0x80 0xFF 字符可能引起问题
    for (auto it = detail::list_const_iterator_t<int64_t>(p_list); it; ++it) {
        ret.push_back(char(*it & 0xFF));
    }
    return varlisp::string_t{std::move(ret)};
}

}  // namespace varlisp
