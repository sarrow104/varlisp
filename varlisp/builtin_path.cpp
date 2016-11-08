#include "eval_visitor.hpp"
#include "object.hpp"

#include <sss/path/glob_path.hpp>
#include <sss/path/glob_path_recursive.hpp>
#include <sss/path/name_filter.hpp>

namespace varlisp {
Object eval_fnamemodify(varlisp::Environment &env, const varlisp::List &args)
{
    Object path = boost::apply_visitor(eval_visitor(env), args.head);

    const std::string *p_path = boost::get<std::string>(&path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error,
                          "fnamemodify requies one path string");
    }
    Object modifier =
        boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    const std::string *p_modifier = boost::get<std::string>(&modifier);
    if (!p_modifier) {
        SSS_POSTION_THROW(std::runtime_error,
                          "fnamemodify requies one path-modifier string");
    }
    return Object(sss::path::modify_copy(*p_path, *p_modifier));
}

// {"glob",        1,  2,  &eval_glob}, //
// 支持1到2个参数；分别是枚举路径和目标规则(可选)；
Object eval_glob(varlisp::Environment &env, const varlisp::List &args)
{
    Object path = boost::apply_visitor(eval_visitor(env), args.head);

    const std::string *p_path = boost::get<std::string>(&path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error, "glob requies one path string");
    }
    std::unique_ptr<sss::path::filter_t> f;
    Object filter;
    if (args.length() > 1) {
        filter = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_filter = boost::get<std::string>(&filter);
        if (!p_filter) {
            SSS_POSTION_THROW(std::runtime_error,
                              "glob: second filter arg must be a string");
        }
        f.reset(new sss::path::name_filter_t(*p_filter));
    }

    varlisp::List ret;
    List *p_list = &ret;

    sss::path::file_descriptor fd;
    sss::path::glob_path gp(".", fd, f.get());
    while (gp.fetch()) {
        if (fd.is_normal_dir()) {
            p_list = p_list->next_slot();
            p_list->head = std::string(fd.get_name()) + sss::path::sp_char;
        }
        else if (fd.is_normal_file()) {
            p_list = p_list->next_slot();
            p_list->head = std::string(fd.get_name());
        }
    }

    return Object(ret);
}

// {"glob-recurse", 1,  3,  &eval_glob_recurse}, //
// 参数同上；第三个可选参数，指查找深度；
Object eval_glob_recurse(varlisp::Environment &env, const varlisp::List &args)
{
    Object path = boost::apply_visitor(eval_visitor(env), args.head);

    const std::string *p_path = boost::get<std::string>(&path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error,
                          "glob-recurse: requies one path string");
    }
    std::unique_ptr<sss::path::filter_t> f;
    if (args.length() > 1) {
        Object arg = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_filter = boost::get<std::string>(&arg);
        if (!p_filter) {
            SSS_POSTION_THROW(
                std::runtime_error,
                "glob-recurse: second filter arg must be a string");
        }
        f.reset(new sss::path::name_filter_t(*p_filter));
    }

    int depth = 0;
    if (args.length() > 2) {
        Object arg =
            boost::apply_visitor(eval_visitor(env), args.tail[0].tail[0].head);
        const int *p_depth = boost::get<int>(&arg);
        if (!p_depth) {
            SSS_POSTION_THROW(std::runtime_error,
                              "glob-recurse: third arg must be an integar");
        }
        depth = *p_depth;
    }

    varlisp::List ret;
    List *p_list = &ret;

    sss::path::file_descriptor fd;
    sss::path::glob_path_recursive gp(*p_path, fd, f.get(), false);
    gp.max_depth(depth);
    while (gp.fetch()) {
        if (fd.is_normal_file()) {
            p_list = p_list->next_slot();
            p_list->head =
                std::string(sss::path::relative_to(fd.get_path(), *p_path));
        }
    }

    return Object(ret);
}

}  // namespace varlisp
