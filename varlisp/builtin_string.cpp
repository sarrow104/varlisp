#include "eval_visitor.hpp"
#include "object.hpp"

#include <sss/spliter.hpp>

namespace varlisp {
/**
 * @brief 拆分字符串
 *
 * @param [in]env
 * @param [in]args 支持两个，参数，分别待切分字符串，和分割字符串；
 *
 * @return 分割之后的列表；
 *
 * TODO 支持正则表达式，确定sep!
 * 需要三个参数；
 * 其中第三个参数是表示正则的的symbol
 */
Object eval_split(varlisp::Environment &env, const varlisp::List &args)
{
    Object content = boost::apply_visitor(eval_visitor(env), args.head);

    const std::string *p_content = boost::get<std::string>(&content);
    if (!p_content) {
        SSS_POSTION_THROW(std::runtime_error, "split requies content to split");
    }
    std::string sep(1, ' ');
    if (args.length() == 2) {
        Object sep_obj =
            boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_sep = boost::get<std::string>(&sep_obj);
        if (!p_sep) {
            SSS_POSTION_THROW(std::runtime_error,
                              "sep using for split must a string");
        }
        sep = *p_sep;
    }
    varlisp::List ret;
    std::string stem;
    if (sep.length() == 1) {
        sss::Spliter sp(*p_content, sep[0]);
        List *p_list = &ret;
        while (sp.fetch_next(stem)) {
            p_list = p_list->next_slot();
            p_list->head = stem;
        }
    }
    else {
        SSS_POSTION_THROW(std::runtime_error,
                          "split: sep.length() >= 2, not support yet!");
    }
    return Object(ret);
}

/**
 * @brief join string list
 *
 * @param [in] env
 * @param [in] args 第一个参数，必须是一个(list)；或者symbol
 *
 * @return
 */
Object eval_join(varlisp::Environment &env, const varlisp::List &args)
{
    const List *p_list = 0;

    Object content = boost::apply_visitor(eval_visitor(env), args.head);

    p_list = boost::get<varlisp::List>(&content);

    if (!p_list) {
        SSS_POSTION_THROW(std::runtime_error, "join: first must a list!");
    }

    std::string sep;
    if (args.length() == 2) {
        Object sep_obj =
            boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_sep = boost::get<std::string>(&sep_obj);
        if (!p_sep) {
            SSS_POSTION_THROW(std::runtime_error, "join: sep must be a string");
        }
        sep = *p_sep;
    }

    std::ostringstream oss;

    bool is_first = true;
    while (p_list && p_list->head.which()) {
        const std::string *p_stem = boost::get<std::string>(&p_list->head);
        Object obj;
        if (!p_stem) {
            obj = boost::apply_visitor(eval_visitor(env), p_list->head);
            p_stem = boost::get<std::string>(&obj);
            if (!p_stem) {
                break;
            }
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
    return Object(oss.str());
}

/**
 * @brief
 *    (substr "target-string" offset)
 *    (substr "target-string" offset length)
 *      -> sub-str
 *
 * @param [in] env
 * @param [in] args
 *
 * @return
 */
Object eval_substr(varlisp::Environment &env, const varlisp::List &args)
{
    Object target = boost::apply_visitor(eval_visitor(env), args.head);
    const std::string *p_content = boost::get<std::string>(&target);
    if (!p_content) {
        SSS_POSTION_THROW(std::runtime_error,
                          "regex-search: need one target string to search");
    }

    int offset = 0;
    Object offset_obj =
        boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    if (const int *p_offset = boost::get<int>(&offset_obj)) {
        offset = *p_offset;
    }
    else if (const double *p_offset = boost::get<double>(&offset_obj)) {
        offset = *p_offset;
    }

    if (offset < 0) {
        offset = 0;
    }
    if (offset > int(p_content->length())) {
        offset = p_content->length();
    }

    int length = -1;
    if (args.length() == 3) {
        Object length_obj =
            boost::apply_visitor(eval_visitor(env), args.tail[0].tail[0].head);
        if (const int *p_length = boost::get<int>(&length_obj)) {
            length = *p_length;
        }
        else if (const double *p_length = boost::get<double>(&length_obj)) {
            length = *p_length;
        }
    }

    if (length < 0) {
        return p_content->substr(offset);
    }
    else {
        return p_content->substr(offset, length);
    }
}

}  // namespace varlisp
