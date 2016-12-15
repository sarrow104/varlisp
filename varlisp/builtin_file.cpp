#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <fstream>

#include <sss/debug/value_msg.hpp>
#include <sss/colorlog.hpp>
#include <sss/path.hpp>
#include <sss/raw_print.hpp>
#include <sss/path/glob_path.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "raw_stream_visitor.hpp"

#include "detail/io.hpp"
#include "detail/buitin_info_t.hpp"
#include "detail/car.hpp"
#include "detail/list_iterator.hpp"
#include "detail/file.hpp"

namespace varlisp {

REGIST_BUILTIN("read-all", 1, 1, eval_read_all,
               "(read-all \"path/to/file\") -> string");

/**
 * @brief (read-all "path/to/file") -> string
 *
 * @param[in] env
 * @param[in] args
 *
 * @return "file-content"
 */
Object eval_read_all(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "read-all";
    Object path;
    const string_t* p_path =
        getTypedValue<string_t>(env, detail::car(args), path);
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
        getTypedValue<string_t>(env, detail::cadr(args), path);
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

    Object obj, objList;
    const Object& firstArg = getAtomicValue(env, detail::car(args), obj);

    if (auto p_list = getQuotedList(env, firstArg, objList)) {
        // NOTE this p_list must be an s-list!
        for (auto it = p_list->begin(); it != p_list->end(); ++it) {
            COLOG_DEBUG(*it);
            boost::apply_visitor(raw_stream_visitor(ofs, env), *it);
        }
    }
    else {
        boost::apply_visitor(raw_stream_visitor(ofs, env), firstArg);
    }

    std::ofstream::pos_type write_cnt = ofs.tellp() - pos;

    COLOG_INFO("(", funcName, ":", sss::raw_string(*p_path), " by ",
               int64_t(write_cnt), "bytes complete)");
    return Object(int64_t(write_cnt));
}

REGIST_BUILTIN("write", 2, 2, eval_write,
               "(write (list) path)\n(write item path)");

Object eval_write(varlisp::Environment& env, const varlisp::List& args)
{
    return eval_write_impl(env, args, false);
}

REGIST_BUILTIN("write-append", 2, 2, eval_write_append,
               "(write-append (list) path)\n(write-append item path)");

Object eval_write_append(varlisp::Environment& env, const varlisp::List& args)
{
    return eval_write_impl(env, args, true);
}

REGIST_BUILTIN("open", 1, 2, eval_open,
               "(open \"path\") -> file_descriptor | nil;\n"
               "(open \"path\" flag) -> file_descriptor | nil");

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
        getTypedValue<string_t>(env, detail::car(args), obj);
    if (!p_path) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requies string path as 1nd argument)");
    }
    std::string std_path = p_path->to_string_view().to_string();
    int64_t flag = O_RDONLY;
    if (args.length() >= 2) {
        const int64_t* p_flag =
            getTypedValue<int64_t>(env, detail::cadr(args), obj);
        if (!p_flag) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": requies int64_t flag as 2nd argument)");
        }
        flag = *p_flag;
    }

    // NOTE 当提供 O_CREAT 掩码的时候，必须使用mode_t 参数！
    // 否则会忽略
    // 00400 user has read permission
    // 00200 user has write permission

    int64_t fd = ::open(std_path.c_str(), flag, S_IRUSR | S_IWUSR);
    COLOG_DEBUG(SSS_VALUE_MSG(fd));
    if (fd == -1) {
        COLOG_ERROR(std::strerror(errno));
    }
    return fd == -1 ? Object{varlisp::Nill{}} : Object{fd};
}

REGIST_BUILTIN("getfdflag", 1, 1, eval_getfdflag,
               "(getfdflag fd) -> flag | nil");

/**
 * @brief
 *     (getfdflag fd) -> flag | nil
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_getfdflag(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "getfdflag";
    Object obj;
    const int64_t* p_fd =
        getTypedValue<int64_t>(env, detail::car(args), obj);
    if (!p_fd) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t fd as 1st argument)");
    }
    int64_t flag = ::fcntl(*p_fd, F_GETFL);
    return flag == -1 ? Object{varlisp::Nill{}} : Object{flag};
}

REGIST_BUILTIN("setfdflag", 2, 2, eval_setfdflag,
               "(setfdflag fd flag) -> errno");

/**
 * @brief
 *     (setfdflag fd flag) -> errno
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_setfdflag(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "setfdflag";
    Object obj;
    const int64_t* p_fd =
        getTypedValue<int64_t>(env, detail::car(args), obj);
    if (!p_fd) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t fd as 1st argument)");
    }
    int64_t fd = *p_fd;
    const int64_t* p_flag =
        getTypedValue<int64_t>(env, detail::cadr(args), obj);
    if (!p_flag) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t flag as 2nd argument)");
    }
    int64_t ec = ::fcntl(fd, F_SETFL, *p_flag);
    return ec == -1 ? int64_t(errno) : int64_t(0);
}

REGIST_BUILTIN("close", 1, 1, eval_close, "(close file_descriptor) -> errno");

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
    const int64_t* p_fd =
        getTypedValue<int64_t>(env, detail::car(args), obj);
    if (!p_fd) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t fd as 1st argument)");
    }
    COLOG_DEBUG(SSS_VALUE_MSG(*p_fd));
    
    // errno；
    // 由于我这个是脚本，不是真正编译程序；也就是说，从产生错误号，到获取
    // 错误号，间隔了多少系统调用？错误号是否被覆盖。
    return ::close(*p_fd) == -1 ? int64_t(errno) : int64_t(0);
}

REGIST_BUILTIN("read-line", 1, 1, eval_read_line,
               "(read-line file_descriptor) -> string | nill");

/**
 * @brief (read-line file_descriptor) -> string | nill
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_read_line(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "read-line";
    Object obj;
    const int64_t* p_fd =
        getTypedValue<int64_t>(env, detail::car(args), obj);
    if (!p_fd) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t fd as 1st argument)");
    }
    std::string line = detail::readline(*p_fd);
    if (line.empty() && errno) {
        return varlisp::Nill{};
    }
    return varlisp::string_t{std::move(line)};
}

REGIST_BUILTIN("read-char", 1, 1, eval_read_char,
               "(read-char file_descriptor) -> int64_t | nill");

/**
 * @brief (read-char file_descriptor) -> int64_t | nill
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_read_char(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "read-char";
    Object obj;
    const int64_t* p_fd =
        getTypedValue<int64_t>(env, detail::car(args), obj);
    if (!p_fd) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t fd as 1st argument)");
    }
    int64_t ch = detail::readchar(*p_fd);
    if (ch == -1 || errno) {
        return varlisp::Nill{};
    }

    return ch;
}

REGIST_BUILTIN("write-char", 2, 2, eval_write_char,
               "(write-char file_descriptor int64_t) -> int64_t | nill");

/**
 * @brief (write-char file_descriptor int64_t) -> int64_t | nill
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_write_char(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "write-char";
    Object obj;
    const int64_t* p_fd =
        getTypedValue<int64_t>(env, detail::car(args), obj);
    if (!p_fd) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t fd as 1st argument)");
    }
    Object chObj;
    const int64_t* p_ch =
        getTypedValue<int64_t>(env, detail::cadr(args), chObj);
    if (!p_ch) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t fd as 2nd argument)");
    }

    int64_t ec = detail::writechar(*p_fd, *p_ch);
    return (ec == -1) ? Object{varlisp::Nill{}} : Object{ec};
}

REGIST_BUILTIN("write-string", 2, 2, eval_write_string,
               "(write-string file_descriptor string) -> int64_t | nill");

/**
 * @brief (write-string file_descriptor int64_t) -> int64_t | nill
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_write_string(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "write-char";
    Object obj;
    const int64_t* p_fd =
        getTypedValue<int64_t>(env, detail::car(args), obj);
    if (!p_fd) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t fd as 1st argument)");
    }
    int64_t fd = *p_fd;
    const varlisp::string_t* p_str =
        getTypedValue<varlisp::string_t>(env, detail::cadr(args), obj);
    if (!p_str) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies string as 2nd argument)");
    }
    int64_t ec = detail::writestring(fd, p_str->to_string_view());
    return (ec == -1) ? Object{varlisp::Nill{}} : Object{ec};
}

REGIST_BUILTIN("get-fd-fname", 1, 1, eval_get_fd_fname,
               "(get-fd-fname fd) -> \"path\" | nill");

Object eval_get_fd_fname(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "get-fd-fname";
    Object obj;
    const int64_t* p_fd =
        getTypedValue<int64_t>(env, detail::car(args), obj);
    if (!p_fd) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t fd as 1st argument)");
    }
    try {
        return string_t(detail::file::get_fname_from_fd(*p_fd));
    }
    catch (std::runtime_error& e) {
        COLOG_ERROR(e.what());
        return Nill{};
    }
}

REGIST_BUILTIN("list-opend-fd", 0, 0, eval_list_opend_fd,
               "; list-opend-fd 枚举打开的文件描述符以及对应的文件名\n"
               "; 并以list的形式返回\n"
               "(list-opend-fd) -> [(fd name)...] | []");

Object eval_list_opend_fd(varlisp::Environment& env, const varlisp::List& args)
{
    const size_t buf_size = 256;
    char dir[buf_size];
    std::snprintf(dir, buf_size - 1, "/proc/%d/fd/", getpid());
    sss::path::file_descriptor fd;
    sss::path::glob_path gp(dir, fd);
    varlisp::List ret;
    auto back_it = varlisp::detail::list_back_inserter<varlisp::Object>(ret);
    while (gp.fetch()) {
        if (!fd.is_normal_file()) {
            continue;
        }
        if (!std::isdigit(fd.get_name()[0])) {
            continue;
        }
        int id = sss::string_cast<int>(fd.get_name());
        *back_it++ = varlisp::List::makeSQuoteList(int64_t(id),
                                                   string_t(detail::file::get_fname_from_fd(id)));
    }
    return varlisp::List::makeSQuoteObj(std::move(ret));
}

}  // namespace
