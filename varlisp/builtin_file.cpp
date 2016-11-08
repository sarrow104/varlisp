#include "builtin_helper.hpp"
#include "eval_visitor.hpp"
#include "object.hpp"
#include "raw_stream_visitor.hpp"

#include <fstream>

#include <sss/colorlog.hpp>
#include <sss/path.hpp>
#include <sss/raw_print.hpp>

namespace varlisp {

/**
 * @brief (read "path/to/file")
 *
 * @param[in] env
 * @param[in] args
 *
 * @return "file-content"
 */
Object eval_read(varlisp::Environment& env, const varlisp::List& args)
{
    Object path = boost::apply_visitor(eval_visitor(env), args.head);
    const std::string* p_path = boost::get<std::string>(&path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error, "read requies a path");
    }
    std::string full_path = sss::path::full_of_copy(*p_path);
    if (sss::path::file_exists(full_path) != sss::PATH_TO_FILE) {
        SSS_POSTION_THROW(std::runtime_error, "path `", *p_path,
                          "` not to file");
    }
    // varlisp::List content;
    // std::string line;
    std::string content;
    sss::path::file2string(full_path, content);

    // 对于小文件，其判断可能出错
    // ensure_utf8(content, "cp936,utf8");

    return content;
}

/**
 * @brief eval_write_impl
 *           (write        (list) path)
 *           (write-append (list) path)
 *           (write        item path)
 *           (write-append item path)
 *
 * 原样，无格式；也没有额外插入"sep-string"
 *
 * @param[in] env
 * @param[in] args
 * @param[in] append
 *
 * @return bytes-writed
 */
Object eval_write_impl(varlisp::Environment& env, const varlisp::List& args,
                       bool append)
{
    Object path = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    const std::string* p_path = boost::get<std::string>(&path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error, "(write: requies path to write)");
    }

    std::string full_path = sss::path::full_of_copy(*p_path);
    sss::path::mkpath(sss::path::dirname(full_path));
    auto bit_op = std::ios_base::out | std::ios_base::binary;
    if (append) {
        bit_op |= std::ios_base::app;
    }
    std::ofstream ofs(full_path, bit_op);
    if (!ofs.good()) {
        SSS_POSTION_THROW(std::runtime_error, "(write: failed open file ",
                          sss::raw_string(*p_path), " to write");
    }

    std::ofstream::pos_type pos = ofs.tellp();

    Object obj;
    const varlisp::List* p_list = getFirstListPtrFromArg(env, args, obj);

    if (p_list) {
        p_list = p_list->next();
        while (p_list && p_list->head.which()) {
            boost::apply_visitor(raw_stream_visitor(ofs, env), p_list->head);
            p_list = p_list->next();
        }
    }
    else {
        boost::apply_visitor(raw_stream_visitor(ofs, env), args.head);
    }

    std::ofstream::pos_type write_cnt = ofs.tellp() - pos;

    if (append) {
        COLOG_INFO("(write-append ", sss::raw_string(*p_path), " by ",
                   int(write_cnt), "bytes complete)");
    }
    else {
        COLOG_INFO("(write ", sss::raw_string(*p_path), "by ", int(write_cnt),
                   "bytes complete)");
    }
    return Object(int(write_cnt));
}

Object eval_write(varlisp::Environment& env, const varlisp::List& args)
{
    return eval_write_impl(env, args, false);
}

Object eval_write_append(varlisp::Environment& env, const varlisp::List& args)
{
    return eval_write_impl(env, args, true);
}

}  // namespace
