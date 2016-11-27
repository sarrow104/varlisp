#include "builtin_helper.hpp"
#include "object.hpp"
#include "raw_stream_visitor.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <fstream>

#include <sss/debug/value_msg.hpp>
#include <sss/colorlog.hpp>
#include <sss/path.hpp>
#include <sss/raw_print.hpp>

namespace varlisp {

/**
 * @brief (read "path/to/file") -> string
 *
 * @param[in] env
 * @param[in] args
 *
 * @return "file-content"
 */
Object eval_read(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "read";
    Object path;
    const string_t* p_path =
        getTypedValue<string_t>(env, args.head, path);
    if (!p_path) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requies a path string as 1st argument)");
    }
    std::string full_path = sss::path::full_of_copy(p_path->to_string());
    if (sss::path::file_exists(full_path) != sss::PATH_TO_FILE) {
        SSS_POSITION_THROW(std::runtime_error, "path `", *p_path,
                          "` not to file");
    }

    std::string content;
    sss::path::file2string(full_path, content);

    return string_t(std::move(content));
}

/**
 * @brief eval_write_impl
 *           (write        (list) path)
 *           (write-append (list) path)
 *           (write        item path)
 *           (write-append item path)
 *              -> written-bytes-count
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
    const char* funcName = append ? "write-append" : "write";
    Object path;
    const string_t* p_path =
        getTypedValue<string_t>(env, args.tail[0].head, path);
    if (!p_path) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requies path as 2nd argument to write)");
    }

    std::string full_path = sss::path::full_of_copy(p_path->to_string());
    sss::path::mkpath(sss::path::dirname(full_path));
    auto bit_op = std::ios_base::out | std::ios_base::binary;
    if (append) {
        bit_op |= std::ios_base::app;
    }
    std::ofstream ofs(full_path, bit_op);
    if (!ofs.good()) {
        SSS_POSITION_THROW(std::runtime_error, "(write: failed open file ",
                          sss::raw_string(*p_path), " to write");
    }

    std::ofstream::pos_type pos = ofs.tellp();

    Object obj;
    const Object& firstArg = getAtomicValue(env, args.head, obj);

    if (const varlisp::List* p_list = boost::get<varlisp::List>(&firstArg)) {
        // NOTE this p_list must be an s-list!
        if (!p_list->is_squote()) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                              ": 1st argument is not a s-list)");
        }
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

    COLOG_INFO("(", funcName, ":", sss::raw_string(*p_path), " by ",
               int(write_cnt), "bytes complete)");
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

/**
 * @brief
 *     (open "path") -> file_descriptor | nil
 *     (open "path" flag) -> file_descriptor | nil
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_open(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "open";
    Object obj;
    const string_t* p_path =
        getTypedValue<string_t>(env, args.head, obj);
    if (!p_path) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requies string path as 1nd argument)");
    }
    std::string std_path = p_path->to_string_view().to_string();
    int flag = O_RDONLY;
    if (args.length() >= 2) {
        const int* p_flag =
            getTypedValue<int>(env, args.tail[0].head, obj);
        if (!p_flag) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": requies int flag as 2nd argument)");
        }
        flag = *p_flag;
    }

    // NOTE 当提供 O_CREAT 掩码的时候，必须使用mode_t 参数！
    // 否则会忽略
    // 00400 user has read permission
    // 00200 user has write permission

    int fd = ::open(std_path.c_str(), flag, S_IRUSR | S_IWUSR);
    COLOG_DEBUG(SSS_VALUE_MSG(fd));
    return fd == -1 ? Object{varlisp::Nill{}} : Object{fd};
}

/**
 * @brief (close file_descriptor) -> errno
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_close(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "close";
    Object obj;
    const int* p_fd =
        getTypedValue<int>(env, args.head, obj);
    if (!p_fd) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int fd as 1st argument)");
    }
    COLOG_DEBUG(SSS_VALUE_MSG(*p_fd));
    
    // errno；
    // 由于我这个是脚本，不是真正编译程序；也就是说，从产生错误号，到获取
    // 错误号，间隔了多少系统调用？错误号是否被覆盖。
    return ::close(*p_fd) == -1 ? errno : 0;
}

}  // namespace
