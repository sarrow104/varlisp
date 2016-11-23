#include "builtin_helper.hpp"
#include "object.hpp"

namespace varlisp {
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
    Object regstr;
    const string_t *p_regstr = getTypedValue<string_t>(env, args.head, regstr);
    if (!p_regstr) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need one string to construct)");
    }
    return sss::regex::CRegex(p_regstr->to_string());
}

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
    Object regobj;
    const sss::regex::CRegex *p_regobj = getTypedValue<sss::regex::CRegex>(env, args.head, regobj);
    if (!p_regobj) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": regex obj)");
    }
    Object target;
    const string_t *p_target = getTypedValue<string_t>(env, args.tail[0].head, target);
    if (!p_target) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need one string to match)");
    }
    return p_regobj->match(p_target->to_string());
}

/**
 * @brief (regex-search reg target offset = 0) -> (list sub0, sub1...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_regex_search(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "regex-search";
    Object regobj;
    const sss::regex::CRegex *p_regobj = getTypedValue<sss::regex::CRegex>(env, args.head, regobj);
    if (!p_regobj) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": regex obj)");
    }

    Object target;
    const string_t *p_target = getTypedValue<string_t>(env, args.tail[0].head, target);
    if (!p_target) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need one string to search)");
    }

    int offset = 0;
    if (args.length() == 3) {
        Object obj;
        if (const int *p_offset = getTypedValue<int>(env, args.tail[0].tail[0].head, obj)) {
            offset = *p_offset;
        }
    }

    if (offset < 0) {
        offset = 0;
    }

    if (offset > int(p_target->length())) {
        offset = p_target->length();
    }

    varlisp::List ret = varlisp::List::makeSQuoteList();

    std::string to_match = p_target->substr(offset).to_string();
    if (p_regobj->match(to_match)) {
        List *p_list = &ret;
        for (int i = 0; i < p_regobj->submatch_count(); ++i) {
            p_list = p_list->next_slot();
            p_list->head = string_t(p_regobj->submatch(i));
        }
    }

    return ret;
}

/**
 * @brief
 *      (regex-replace reg-obj target fmt)->string
 *
 * @param [in]env
 * @param [in]args
 *
 * @return
 */
Object eval_regex_replace(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "regex-replace";
    Object regobj;
    const sss::regex::CRegex *p_regobj = getTypedValue<sss::regex::CRegex>(env, args.head, regobj);
    if (!p_regobj) {
        SSS_POSITION_THROW(std::runtime_error, "regex-replace: regex obj");
    }

    Object target;
    const string_t *p_target = getTypedValue<string_t>(env, args.tail[0].head, target);
    if (!p_target) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need one string to replace)");
    }

    Object fmtobj;
    const string_t *p_fmt = getTypedValue<string_t>(env, args.tail[0].tail[0].head, fmtobj);
    if (!p_fmt) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need fmt string at 3rd)");
    }

    std::string out;
    p_regobj->substitute(p_target->to_string(), p_fmt->to_string(), out);
    return string_t(std::move(out));
}

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
    const sss::regex::CRegex *p_regobj = getTypedValue<sss::regex::CRegex>(env, args.head, regobj);
    if (!p_regobj) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": regex obj)");
    }

    Object target;
    const string_t *p_target = getTypedValue<string_t>(env, args.tail[0].head, target);
    if (!p_target) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need one string to replace)");
    }

    // FIXME  换 pcre？
    std::string target_str = p_target->to_string();
    const char *str_beg = target_str.c_str();
    varlisp::List ret = varlisp::List::makeSQuoteList();
    List *p_list = &ret;

    while (str_beg && *str_beg && p_regobj->match(str_beg)) {
        p_list = p_list->next_slot();

        // std::string(str_beg, str_beg + p_regobj->submatch_start(0));
        // string_t stem = ;
        p_list->head = p_target->substr(str_beg - target_str.c_str(), p_regobj->submatch_start(0));

        str_beg += p_regobj->submatch_end(0);
    }

    if (str_beg && *str_beg) {
        p_list = p_list->next_slot();
        p_list->head = p_target->substr(str_beg - target_str.c_str());
    }

    return ret;
}

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
    Object regobj;
    const sss::regex::CRegex *p_regobj = getTypedValue<sss::regex::CRegex>(env, args.head, regobj);
    if (!p_regobj) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": regex obj)");
    }

    Object target;
    const string_t *p_target = getTypedValue<string_t>(env, args.tail[0].head, target);
    if (!p_target) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": need one string to search)");
    }
    const string_t *p_fmt = 0;
    if (args.length() == 3) {
        Object fmtobj;
        p_fmt = getTypedValue<string_t>(env, args.tail[0].tail[0].head, fmtobj);
    }

    // FIXME TODO pcre
    std::string content = p_target->to_string();
    const char *str_beg = content.c_str();
    varlisp::List ret = varlisp::List::makeSQuoteList();
    List *p_list = &ret;

    while (str_beg && *str_beg && p_regobj->match(str_beg)) {
        p_list = p_list->next_slot();
        str_beg += p_regobj->submatch_start(0);

        if (p_fmt) {
            std::ostringstream oss;
            for (std::string::size_type i = 0; i != p_fmt->length(); ++i) {
                if (p_fmt->at(i) == '\\' && i + 1 != p_fmt->length() &&
                    std::isdigit(p_fmt->at(i + 1))) {
                    int index = p_fmt->at(i + 1) - '0';
                    oss.write(str_beg + p_regobj->submatch_start(index),
                              p_regobj->submatch_consumed(index));
                    ++i;
                }
                else {
                    oss << p_fmt->at(i);
                }
            }
            p_list->head = Object{string_t{std::move(oss.str())}};
        }
        else {
            p_list->head = p_target->substr(str_beg - content.c_str(), p_regobj->submatch_consumed(0));
                // std::string(str_beg, str_beg + p_regobj->submatch_consumed(0));
        }
        str_beg += p_regobj->submatch_consumed(0);
    }

    return ret;
}

}  // namespace varlisp
