#include <array>

#include <sss/debug/value_msg.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/list_iterator.hpp"
#include "../detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("regex", 1, 1, eval_regex,
               "; regex 生成基于Google/re2 的正则表达式对象\n"
               "; 完整的语法描述，见$root/re2-syntax.html 和 $root/re2-syntax.txt\n"
               "(regex \"regex-string\") -> regex-obj");

/**
 * @brief (regex "regex-string") -> regex-obj
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_regex(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "regex";
    std::array<Object, 1> objs;
    const string_t *p_regstr =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    return std::make_shared<RE2>(*p_regstr);
}

REGIST_BUILTIN("regex-match", 2, 2, eval_regex_match,
               "(regex-match reg-obj target-string) -> bool");

/**
 * @brief (regex-match reg-obj target-string) -> bool
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_regex_match(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "regex-match";
    std::array<Object, 2> objs;
    const varlisp::regex_t *p_regobj =
        requireTypedValue<varlisp::regex_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t *p_target =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    return RE2::PartialMatch(*p_target, *(*p_regobj));
}

REGIST_BUILTIN("regex-search", 2, 3, eval_regex_search,
               "(regex-search reg target offset = 0) -> (list sub0, sub1 ...)");

/**
 * @brief (regex-search reg target offset = 0) -> (list sub0, sub1 ...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_regex_search(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "regex-search";
    std::array<Object, 3> objs;
    const varlisp::regex_t *p_regobj =
        requireTypedValue<varlisp::regex_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t *p_target =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    int64_t offset = 0;
    if (args.length() == 3) {
        offset = *requireTypedValue<int64_t>(env, args.nth(2), objs[2], funcName, 2, DEBUG_INFO);
    }

    if (offset < 0) {
        offset = 0;
    }

    if (offset > int64_t(p_target->length())) {
        offset = p_target->length();
    }

    varlisp::List ret = varlisp::List::makeSQuoteList();

    std::vector<re2::StringPiece> sub_matches;
    sub_matches.resize(10);
    if ((*p_regobj)->Match(*p_target, offset, p_target->size(), RE2::UNANCHORED,
                           &sub_matches[0], sub_matches.size()))
    {
        auto back_it = detail::list_back_inserter<varlisp::string_t>(ret);
        while (!sub_matches.empty() && sub_matches.back().empty()) {
            sub_matches.pop_back();
        }
        for (const auto& piece : sub_matches) {
            *back_it++ = p_target->substr(sss::string_view(piece.data(), piece.size()));
        }
    }

    return ret;
}

REGIST_BUILTIN("regex-replace", 2, 3, eval_regex_replace,
               "; regex-replace 如果不提供fmt参数，则表示删除匹配到的部分文字\n"
               "(regex-replace reg-obj target) -> string\n"
               "(regex-replace reg-obj target fmt) -> string");

/**
 * @brief
 *
 * @param [in]env
 * @param [in]args
 *
 * @return
 */
Object eval_regex_replace(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "regex-replace";
    std::array<Object, 3> tmpObjs;
    const varlisp::regex_t *p_regobj =
        requireTypedValue<varlisp::regex_t>(env, args.nth(0), tmpObjs[0], funcName, 0, DEBUG_INFO);

    const string_t *p_target =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), tmpObjs[1], funcName, 1, DEBUG_INFO);

    re2::StringPiece fmt = "";
    if (args.length() == 3) {
        const string_t *p_fmt =
            requireTypedValue<varlisp::string_t>(env, args.nth(2), tmpObjs[2], funcName, 2, DEBUG_INFO);
        fmt = *p_fmt;
    }

    std::string out = *p_target->gen_shared();
    RE2::GlobalReplace(&out, *(*p_regobj), fmt);
    return string_t(std::move(out));
}

REGIST_BUILTIN(
    "regex-split", 2, 2, eval_regex_split,
    "(regex-split sep-reg \"target-string\") -> (list stem1 stem2 ...)");

/**
 * @brief
 *      (regex-split sep-reg "target-string") -> (list stem1 stem2 ...)
 *
 * @param [in] env
 * @param [in] args
 *
 * @return
 */
Object eval_regex_split(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "regex-split";
    Object regobj;
    const varlisp::regex_t *p_regobj =
        getTypedValue<varlisp::regex_t>(env, detail::car(args), regobj);
    if (!p_regobj) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": regex obj)");
    }

    Object target;
    const string_t *p_target =
        getTypedValue<string_t>(env, detail::cadr(args), target);
    if (!p_target) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need one string to replace)");
    }

    varlisp::List ret = varlisp::List::makeSQuoteList();
    auto back_it = detail::list_back_inserter<varlisp::string_t>(ret);

    re2::StringPiece sub_matches;
    size_t start_pos = 0;
    while ((*p_regobj)->Match(*p_target, start_pos, p_target->size(), RE2::UNANCHORED, &sub_matches, 1)) {
        *back_it++ = p_target->substr(start_pos, sub_matches.data() - p_target->data() - start_pos);
        start_pos = sub_matches.end() - p_target->data();
    }

    if (start_pos < p_target->size()) {
        *back_it++ = p_target->substr(start_pos);
    }

    return ret;
}

REGIST_BUILTIN("regex-collect", 2, 3, eval_regex_collect,
               "(regex-collect reg \"target-string\")\n"
               "(regex-collect reg \"target-string\" \"fmt-string\") ->"
               " (list matched-sub1 matched-sub2 ...)");

/**
 * @brief
 *      (regex-collect reg "target-string")
 *      (regex-collect reg "target-string" "fmt-string")
 *          -> (list matched-sub1 matched-sub2 ...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_regex_collect(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "regex-collect";
    std::array<Object, 3> objs;
    const varlisp::regex_t *p_regobj =
        requireTypedValue<varlisp::regex_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    if (!(*p_regobj)) {
        SSS_POSITION_THROW(std::runtime_error, "not init regex-obj!");
    }

    const string_t *p_target =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    const string_t *p_fmt = 0;
    if (args.length() == 3) {
        p_fmt = requireTypedValue<varlisp::string_t>(env, args.nth(2), objs[2], funcName, 2, DEBUG_INFO);
    }

    varlisp::List ret = varlisp::List::makeSQuoteList();
    auto back_it = detail::list_back_inserter<varlisp::string_t>(ret);

    std::string fmt_error_msg;
    if (p_fmt && !(*p_regobj)->CheckRewriteString(*p_fmt, &fmt_error_msg)) {
        SSS_POSITION_THROW(std::runtime_error, fmt_error_msg);
    }

    std::vector<re2::StringPiece> sub_matches;
    re2::StringPiece rewrite = p_fmt ? *p_fmt : re2::StringPiece("\\0");
    sub_matches.resize(RE2::MaxSubmatch(rewrite) + 1);

    size_t start_pos = 0;
    while ((*p_regobj)->Match(*p_target, start_pos, p_target->size(),
                              RE2::UNANCHORED, &sub_matches[0],
                              sub_matches.size()))
    {
        std::string out;

        if ((*p_regobj)->Rewrite(&out, rewrite, &sub_matches[0], sub_matches.size())) {
            *back_it++ = string_t(std::move(out));
        }
        start_pos = sub_matches.front().end() - p_target->data();
    }

    return ret;
}

}  // namespace varlisp
