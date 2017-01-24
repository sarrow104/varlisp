#include <array>

#include <sss/path/glob_path.hpp>
#include <sss/path/glob_path_recursive.hpp>
#include <sss/path/name_filter.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/list_iterator.hpp"
#include "../detail/varlisp_env.hpp"

namespace varlisp {

REGIST_BUILTIN(
    "path-fnamemodify", 2, 2, eval_path_fnamemodify,
    "; fnamemodify 类vim fnamemodify功能；\n"
    "; Examples, when the file name is \"src/version.c\", current dir\n"
    "; \"/home/mool/vim\": >\n"
    "; (注意 替换部分的语法，暂不支持)\n"
    "; :p			/home/mool/vim/src/version.c\n"
    "; :p:.				       src/version.c\n"
    "; :p:~				 ~/vim/src/version.c\n"
    "; :h				       src\n"
    "; :p:h			/home/mool/vim/src\n"
    "; :p:h:h		/home/mool/vim\n"
    "; :t					   version.c\n"
    "; :p:t					   version.c\n"
    "; :r				       src/version\n"
    "; :p:r			/home/mool/vim/src/version\n"
    "; :t:r					   version\n"
    "; :e						   c\n"
    "; :s?version?main?		       src/main.c\n"
    "; :s?version?main?:p	/home/mool/vim/src/main.c\n"
    "; :p:gs?/?\\?		\\home\\mool\\vim\\src\\version.c\n"

    "(path-fnamemodify \"path/string\" \"path modifier\") -> \"modified-fname\"");

/**
 * @brief (path-fnamemodify "path/string" "path modifier") -> "modified-fname"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_path_fnamemodify(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "path-fnamemodify";
    std::array<Object, 2> objs;
    Object path;
    const string_t *p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t *p_modifier =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    std::string mod_name = sss::path::modify_copy(*p_path->gen_shared(), *p_modifier->gen_shared());
    return Object(string_t(std::move(mod_name)));
}

REGIST_BUILTIN("path-append", 2, 2, eval_path_append,
               "; path-append 路径附加操作\n"
               "(path-append part1 part2) -> part1/part2")

Object eval_path_append(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "path-append";
    std::array<Object, 2> objs;
    const string_t *p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const string_t *p_toAppend =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    std::string out_name = sss::path::full_of_copy(varlisp::detail::envmgr::expand(*p_path->gen_shared()));
    sss::path::append(out_name, *p_toAppend->gen_shared());
    return string_t(std::move(out_name));
}

REGIST_BUILTIN("glob", 1, 2, eval_glob,
               "; glob 枚举目标路径下的文件、文件夹；\n"
               "; 支持1到2个参数；分别是枚举路径和目标规则(可选)；\n"
               "(glob \"paht/to/explorer\") -> '(\"fname1\", \"fname2\", ...)\n"
               "(glob \"paht/to/explorer\" \"fname-filter\") ->"
               " '(\"fname1\", \"fname2\", ...)");

/**
 * @brief
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_glob(varlisp::Environment &env, const varlisp::List &args)
{
    const char *funcName = "glob";
    std::array<Object, 2> objs;
    const string_t *p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::unique_ptr<sss::path::filter_t> f;
    if (args.length() > 1) {
        const string_t *p_filter =
            requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);
        f.reset(new sss::path::name_filter_t(*p_filter->gen_shared()));
    }

    varlisp::List ret = varlisp::List::makeSQuoteList();
    auto ret_it = detail::list_back_inserter<Object>(ret);
    
    sss::path::file_descriptor fd;
    std::string path = sss::path::full_of_copy(varlisp::detail::envmgr::expand(*p_path->gen_shared()));
    sss::path::glob_path gp(path, fd, f.get());
    while (gp.fetch()) {
        if (fd.is_normal_dir()) {
            std::string name = fd.get_name();
            // name += sss::path::sp_char;
            *ret_it++ = string_t(std::move(name));
        }
        else if (fd.is_normal_file()) {
            *ret_it++ = string_t(fd.get_name());
        }
    }

    return Object(ret);
}

REGIST_BUILTIN(
    "file?", 1, 1, eval_file_q,
    "; file? 询问路径指向的文件是否存在\n"
    "(file? \"path/to/file\") -> boolean");

Object eval_file_q(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "file?";
    std::array<Object, 1> objs;
    const string_t *p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::string path = sss::path::full_of_copy(varlisp::detail::envmgr::expand(*p_path->gen_shared()));
    return sss::path::file_exists(path) == sss::PATH_TO_FILE;
}

REGIST_BUILTIN(
    "directory?", 1, 1, eval_directory_q,
    "; directory? 询问路径指向的文件夹是否存在\n"
    "(directory? \"path/to/directory\") -> boolean");

Object eval_directory_q(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "directory?";
    std::array<Object, 1> objs;
    const string_t *p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::string path = sss::path::full_of_copy(varlisp::detail::envmgr::expand(*p_path->gen_shared()));
    return sss::path::file_exists(path) == sss::PATH_TO_DIRECTORY;
}

REGIST_BUILTIN(
    "glob-recurse", 1, 3, eval_glob_recurse,
    "; glob-recurse 递归枚举目标路径下的文件、文件夹\n"
    "; 参数同 glob；第三个可选参数，指查找深度；\n"
    "(glob-recurse \"paht/to/explorer\") -> '(\"fname1\", \"fname2\", ...)\n"
    "(glob-recurse \"paht/to/explorer\" \"fname-filter\") ->"
    " '(\"fname1\", \"fname2\", ...)\n"
    "(glob-recurse \"paht/to/explorer\" \"fname-filter\" depth) ->"
    " '(\"fname1\", \"fname2\", ...)");

/**
 * @brief
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_glob_recurse(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "glob-recurse";
    std::array<Object, 3> objs;
    const string_t *p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::unique_ptr<sss::path::filter_t> f;
    if (args.length() > 1) {
        Object filter;
        const string_t *p_filter =
            requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);
        f.reset(new sss::path::name_filter_t(*p_filter->gen_shared()));
    }

    int depth = 0;
    if (args.length() > 2) {
        Object arg;
        const int64_t *p_depth =
            requireTypedValue<int64_t>(env, args.nth(2), objs[2], funcName, 2, DEBUG_INFO);
        depth = *p_depth;
    }

    varlisp::List ret = varlisp::List::makeSQuoteList();
    auto ret_it = detail::list_back_inserter<Object>(ret);

    sss::path::file_descriptor fd;
    std::string path = sss::path::full_of_copy(varlisp::detail::envmgr::expand(*p_path->gen_shared()));
    sss::path::glob_path_recursive gp(path, fd, f.get(), false);
    gp.max_depth(depth);
    while (gp.fetch()) {
        if (fd.is_normal_file()) {
            *ret_it++ = string_t(sss::path::relative_to(fd.get_path(), path));
        }
    }

    return Object(ret);
}

REGIST_BUILTIN("expand", 1, 1, eval_expand,
               "(expand \"$var-path\") -> \"expand-path\"")

Object eval_expand(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "expand";
    std::array<Object, 1> objs;
    const string_t *p_path = requireTypedValue<varlisp::string_t>(
        env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    return string_t(varlisp::detail::envmgr::expand(*p_path->gen_shared()));
}

}  // namespace varlisp
