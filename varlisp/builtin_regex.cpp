#include "eval_visitor.hpp"
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
    Object content = boost::apply_visitor(eval_visitor(env), args.head);
    const std::string *p_content = boost::get<std::string>(&content);
    if (!p_content) {
        SSS_POSTION_THROW(std::runtime_error,
                          "regex: need one string to construct");
    }
    return sss::regex::CRegex(*p_content);
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
    Object content = boost::apply_visitor(eval_visitor(env), args.head);
    sss::regex::CRegex *p_reg = boost::get<sss::regex::CRegex>(&content);
    if (!p_reg) {
        SSS_POSTION_THROW(std::runtime_error, "regex-match: regex obj");
    }
    Object target = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    const std::string *p_content = boost::get<std::string>(&target);
    if (!p_content) {
        SSS_POSTION_THROW(std::runtime_error,
                          "regex-match: need one string to match");
    }
    return p_reg->match(p_content->c_str());
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
    Object reg_obj = boost::apply_visitor(eval_visitor(env), args.head);
    sss::regex::CRegex *p_reg = boost::get<sss::regex::CRegex>(&reg_obj);
    if (!p_reg) {
        SSS_POSTION_THROW(std::runtime_error, "regex-search: regex obj");
    }

    Object target = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    const std::string *p_content = boost::get<std::string>(&target);
    if (!p_content) {
        SSS_POSTION_THROW(std::runtime_error,
                          "regex-search: need one target string to search");
    }

    int offset = 0;
    if (args.length() == 3) {
        Object offset_obj =
            boost::apply_visitor(eval_visitor(env), args.tail[0].tail[0].head);
        if (const int *p_offset = boost::get<int>(&offset_obj)) {
            offset = *p_offset;
        }
        else if (const double *p_offset = boost::get<double>(&offset_obj)) {
            offset = *p_offset;
        }
    }

    if (offset < 0) {
        offset = 0;
    }

    if (offset > int(p_content->length())) {
        offset = p_content->length();
    }

    varlisp::List ret = varlisp::List::makeSQuoteList();

    if (p_reg->match(p_content->c_str() + offset)) {
        List *p_list = &ret;
        for (int i = 0; i < p_reg->submatch_count(); ++i) {
            p_list = p_list->next_slot();
            p_list->head = p_reg->submatch(i);
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
    Object reg_obj = boost::apply_visitor(eval_visitor(env), args.head);
    sss::regex::CRegex *p_reg = boost::get<sss::regex::CRegex>(&reg_obj);
    if (!p_reg) {
        SSS_POSTION_THROW(std::runtime_error, "regex-replace: regex obj");
    }

    Object target = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    const std::string *p_content = boost::get<std::string>(&target);
    if (!p_content) {
        SSS_POSTION_THROW(std::runtime_error,
                          "regex-replace: need one target string to replace");
    }

    Object fmt_obj =
        boost::apply_visitor(eval_visitor(env), args.tail[0].tail[0].head);
    const std::string *p_fmt = boost::get<std::string>(&fmt_obj);
    if (!p_fmt) {
        SSS_POSTION_THROW(std::runtime_error,
                          "regex-replace: need one fmt string to replace");
    }

    std::string out;
    p_reg->substitute(*p_content, *p_fmt, out);
    return out;
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
    Object reg_obj = boost::apply_visitor(eval_visitor(env), args.head);
    sss::regex::CRegex *p_reg = boost::get<sss::regex::CRegex>(&reg_obj);
    if (!p_reg) {
        SSS_POSTION_THROW(std::runtime_error, "regex-replace: regex obj");
    }

    Object target = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    const std::string *p_content = boost::get<std::string>(&target);
    if (!p_content) {
        SSS_POSTION_THROW(std::runtime_error,
                          "regex-replace: need one target string to replace");
    }

    const char *str_beg = p_content->c_str();
    varlisp::List ret = varlisp::List::makeSQuoteList();
    List *p_list = &ret;

    while (str_beg && *str_beg && p_reg->match(str_beg)) {
        p_list = p_list->next_slot();

        p_list->head = std::string(str_beg, str_beg + p_reg->submatch_start(0));

        str_beg += p_reg->submatch_end(0);
    }

    if (str_beg && *str_beg) {
        p_list = p_list->next_slot();
        p_list->head = std::string(str_beg);
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
    Object reg_obj = boost::apply_visitor(eval_visitor(env), args.head);
    sss::regex::CRegex *p_reg = boost::get<sss::regex::CRegex>(&reg_obj);
    if (!p_reg) {
        SSS_POSTION_THROW(std::runtime_error, "regex-replace: regex obj");
    }

    Object target = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    const std::string *p_content = boost::get<std::string>(&target);
    if (!p_content) {
        SSS_POSTION_THROW(std::runtime_error,
                          "regex-replace: need one target string to replace");
    }
    const std::string *p_fmt = 0;
    if (args.length() == 3) {
        Object fmt_obj =
            boost::apply_visitor(eval_visitor(env), args.tail[0].tail[0].head);
        p_fmt = boost::get<std::string>(&fmt_obj);
    }

    const char *str_beg = p_content->c_str();
    varlisp::List ret = varlisp::List::makeSQuoteList();
    List *p_list = &ret;

    while (str_beg && *str_beg && p_reg->match(str_beg)) {
        p_list = p_list->next_slot();
        str_beg += p_reg->submatch_start(0);

        if (p_fmt) {
            std::ostringstream oss;
            for (std::string::size_type i = 0; i != p_fmt->length(); ++i) {
                if (p_fmt->at(i) == '\\' && i + 1 != p_fmt->length() &&
                    std::isdigit(p_fmt->at(i + 1))) {
                    int index = p_fmt->at(i + 1) - '0';
                    oss.write(str_beg + p_reg->submatch_start(index),
                              p_reg->submatch_consumed(index));
                    ++i;
                }
                else {
                    oss << p_fmt->at(i);
                }
            }
            p_list->head = oss.str();
        }
        else {
            p_list->head =
                std::string(str_beg, str_beg + p_reg->submatch_consumed(0));
        }
        str_beg += p_reg->submatch_consumed(0);
    }

    return ret;
}

}  // namespace varlisp
