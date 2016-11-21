#include "builtin_helper.hpp"
#include "object.hpp"

#include <sss/path/glob_path.hpp>
#include <sss/path/glob_path_recursive.hpp>
#include <sss/path/name_filter.hpp>

namespace varlisp {
/**
 * @brief (fnamemodify "path/string" "path modifier") -> "modified-fname"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_fnamemodify(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "fnamemodify";
    Object path;
    const string_t *p_path =
        getTypedValue<string_t>(env, args.head, path);
    if (!p_path) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requies one path string at 1st)");
    }
    Object modifier;
    const string_t *p_modifier =
        getTypedValue<string_t>(env, args.tail[0].head, modifier);
    if (!p_modifier) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requies one path-modifier string at 2nd)");
    }
    std::string mod_name = sss::path::modify_copy(p_path->to_string(), p_modifier->to_string());
    return Object(string_t(std::move(mod_name)));
}

// {"glob",        1,  2,  &eval_glob}, //
// 支持1到2个参数；分别是枚举路径和目标规则(可选)；
/**
 * @brief
 *       (glob "paht/to/explorer") -> '("fname1", "fname2", ...)
 *       (glob "paht/to/explorer" "fname-filter") -> '("fname1", "fname2", ...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_glob(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "glob";
    Object path;
    const string_t *p_path =
        getTypedValue<string_t>(env, args.head, path);

    if (!p_path) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          " requies one path string at 1st)");
    }

    std::unique_ptr<sss::path::filter_t> f;
    if (args.length() > 1) {
        Object filter;
        const string_t *p_filter =
            getTypedValue<string_t>(env, args.tail[0].head, filter);
        if (!p_filter) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                              ": requires filter string as 2nd argument)");
        }
        f.reset(new sss::path::name_filter_t(p_filter->to_string()));
    }

    varlisp::List ret = varlisp::List::makeSQuoteList();
    List *p_list = &ret;

    sss::path::file_descriptor fd;
    sss::path::glob_path gp(".", fd, f.get());
    while (gp.fetch()) {
        if (fd.is_normal_dir()) {
            p_list = p_list->next_slot();
            std::string name = fd.get_name();
            name += sss::path::sp_char;
            p_list->head = string_t(std::move(name));
        }
        else if (fd.is_normal_file()) {
            p_list = p_list->next_slot();
            p_list->head = string_t(fd.get_name());
        }
    }

    return Object(ret);
}

// {"glob-recurse", 1,  3,  &eval_glob_recurse}, //
// 参数同上；第三个可选参数，指查找深度；
/**
 * @brief
 *      (glob-recurse "paht/to/explorer") -> '("fname1", "fname2", ...)
 *      (glob-recurse "paht/to/explorer" "fname-filter") -> '("fname1",
 * "fname2", ...)
 *      (glob-recurse "paht/to/explorer" "fname-filter" depth) -> '("fname1",
 * "fname2", ...)
 *
 * @param env
 * @param args
 *
 * @return
 */
Object eval_glob_recurse(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "glob-recurse";
    Object path;
    const string_t *p_path =
        getTypedValue<string_t>(env, args.head, path);
    if (!p_path) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": requies one path string)");
    }
    std::unique_ptr<sss::path::filter_t> f;
    if (args.length() > 1) {
        Object filter;
        const string_t *p_filter =
            getTypedValue<string_t>(env, args.tail[0].head, filter);
        if (!p_filter) {
            SSS_POSITION_THROW(
                std::runtime_error,
                "(", funcName, ": second filter arg must be a string)");
        }
        f.reset(new sss::path::name_filter_t(p_filter->to_string()));
    }

    int depth = 0;
    if (args.length() > 2) {
        Object arg;
        const int *p_depth = getTypedValue<int>(env, args.tail[0].tail[0].head, arg);
        if (!p_depth) {
            SSS_POSITION_THROW(std::runtime_error,
                              "(", funcName, ": third arg must be an integar)");
        }
        depth = *p_depth;
    }

    varlisp::List ret = varlisp::List::makeSQuoteList();
    List *p_list = &ret;

    sss::path::file_descriptor fd;
    sss::path::glob_path_recursive gp(p_path->to_string(), fd, f.get(), false);
    gp.max_depth(depth);
    while (gp.fetch()) {
        if (fd.is_normal_file()) {
            p_list = p_list->next_slot();
            p_list->head = string_t(sss::path::relative_to(fd.get_path(), p_path->to_string()));
        }
    }

    return Object(ret);
}

}  // namespace varlisp
