#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <fstream>
#include <cstdlib>

#include <sss/debug/value_msg.hpp>
#include <sss/colorlog.hpp>
#include <sss/path.hpp>
#include <sss/raw_print.hpp>
#include <sss/path/glob_path.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../raw_stream_visitor.hpp"

#include "../detail/io.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/list_iterator.hpp"
#include "../detail/file.hpp"
#include "../detail/varlisp_env.hpp"

namespace varlisp {

REGIST_BUILTIN("read-all", 1, 1, eval_read_all,
               "(read-all fd) -> string\n"
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
    Object tmp;

    const Object& resRef = varlisp::getAtomicValue(env, args.nth(0), tmp);

    std::string content;
    if (const string_t* p_path = boost::get<varlisp::string_t>(&resRef)) {
        std::string full_path = sss::path::full_of_copy(*p_path->gen_shared());
        if (sss::path::file_exists(full_path) != sss::PATH_TO_FILE) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName, " `", *p_path,
                               "` not to file)");
        }
        sss::path::file2string(full_path, content);

    } else if (const int64_t* p_fd = boost::get<int64_t>(&resRef)) {
        content = detail::readall(*p_fd);
    } else {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName, " `", resRef,
                           "` must be path/to/file:string or fd:int to already opened file)");
    }

    return string_t(std::move(content));
}

/**
 * @brief eval_write_impl
 *           (write        path (list))
 *           (write-append path (list))
 *           (write        path item...)
 *           (write-append path item...)
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
    std::array<Object, 2> objs;
    const string_t* p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::string full_path = sss::path::full_of_copy(*p_path->gen_shared());
    sss::path::mkpath(sss::path::dirname(full_path));
    auto bit_op = std::ios_base::out | std::ios_base::binary;
    if (append) {
        bit_op |= std::ios_base::app;
    }
    std::ofstream ofs(full_path, bit_op);
    if (!ofs.good()) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": failed open file ", sss::raw_string(*p_path),
                           " to write");
    }

    std::ofstream::pos_type pos = ofs.tellp();

    Object objList;
    const Object& firstArg = getAtomicValue(env, args.nth(1), objs[1]);

    if (auto p_list = getQuotedList(env, firstArg, objList)) {
        // NOTE this p_list must be an s-list!
        for (auto it = p_list->begin(); it != p_list->end(); ++it) {
            COLOG_DEBUG(*it);
            boost::apply_visitor(raw_stream_visitor(ofs, env), *it);
        }
    }
    else {
        boost::apply_visitor(raw_stream_visitor(ofs, env), firstArg);
        for (size_t i = 2; i < args.length(); ++i) {
            const Object& arg = getAtomicValue(env, args.nth(i), objs[1]);
            boost::apply_visitor(raw_stream_visitor(ofs, env), arg);
        }
    }

    std::ofstream::pos_type write_cnt = ofs.tellp() - pos;

    COLOG_INFO("(", funcName, ":", sss::raw_string(*p_path), " by ",
               int64_t(write_cnt), "bytes complete)");
    return Object(int64_t(write_cnt));
}

REGIST_BUILTIN("write", 2, -1, eval_write,
               "(write path (list))\n"
               "(write path item...)");

Object eval_write(varlisp::Environment& env, const varlisp::List& args)
{
    return eval_write_impl(env, args, false);
}

REGIST_BUILTIN("write-append", 2, -1, eval_write_append,
               "(write-append path (list))\n"
               "(write-append path item...)");

Object eval_write_append(varlisp::Environment& env, const varlisp::List& args)
{
    return eval_write_impl(env, args, true);
}

REGIST_BUILTIN("opentmp", 0, 0, eval_opentmp,
               "(opentmp) -> file_descriptor | nil");

Object eval_opentmp(varlisp::Environment& env, const varlisp::List& args)
{
    (void)env;
    (void)args;

    char tpl[] = "prefixXXXXXX";
    int64_t fd = ::mkstemp(tpl);
    if (fd != -1) {
        varlisp::detail::file::register_fd(fd);
    }
    return fd == -1 ? Object{varlisp::Nill{}} : Object{fd};
}

REGIST_BUILTIN("open", 1, -1, eval_open,
               "(open \"path\") -> file_descriptor | nil\n"
               "(open \"path\" flag) -> file_descriptor | nil\n"
               "(open \"path\" flag1 flag2...) -> file_descriptor | nil");

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
    std::array<Object, 2> objs;
    const string_t* p_path = requireTypedValue<varlisp::string_t>(
        env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    auto std_path = p_path->gen_shared();
    int64_t flag = O_RDONLY;
    if (args.length() >= 2) {
        flag = *requireTypedValue<int64_t>(env, args.nth(1), objs[1], funcName,
                                           1, DEBUG_INFO);

        for (size_t i = 2; i < args.length(); ++i) {
            flag |= *requireTypedValue<int64_t>(env, args.nth(i), objs[1],
                                                funcName, i, DEBUG_INFO);
        }
    }

    // NOTE 当提供 O_CREAT 掩码的时候，必须使用mode_t 参数！
    // 否则会忽略
    // 00400 user has read permission
    // 00200 user has write permission

    int64_t fd = ::open(std_path->c_str(), flag, S_IRUSR | S_IWUSR);
    COLOG_DEBUG(SSS_VALUE_MSG(fd));
    if (fd == -1) {
        COLOG_ERROR(std::strerror(errno));
    }
    return fd == -1 ? Object{varlisp::Nill{}} : Object{fd};
}

REGIST_BUILTIN("lseek", 3, 3, eval_lseek,
               "(lseek fd offset whence) -> offset | nil");

/**
 * @brief
 *     (lseek fd offset whence) -> offset | nil
 *
 *     whence: SEEK_SET | SEEK_CUR | SEEK_END
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_lseek(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "lseek";
    std::array<Object, 3> objs;

    int64_t fd = *requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    int64_t offset = *requireTypedValue<int64_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);
    int64_t whence = *requireTypedValue<int64_t>(env, args.nth(2), objs[2], funcName, 2, DEBUG_INFO);

    int64_t res = ::lseek(fd, offset, whence);
    COLOG_DEBUG(SSS_VALUE_MSG(res));
    if (res == -1) {
        COLOG_ERROR(std::strerror(errno));
    }
    return res == -1 ? Object{varlisp::Nill{}} : Object{res};
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
        requireTypedValue<int64_t>(env, args.nth(0), obj, funcName, 0, DEBUG_INFO);

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
    std::array<Object, 2> objs;
    const int64_t* p_fd =
        requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const int64_t* p_flag =
        requireTypedValue<int64_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    int64_t ec = ::fcntl(*p_fd, F_SETFL, *p_flag);
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
        requireTypedValue<int64_t>(env, args.nth(0), obj, funcName, 0, DEBUG_INFO);

    COLOG_DEBUG(SSS_VALUE_MSG(*p_fd));
    if (*p_fd != -1) {
        varlisp::detail::file::unregister_fd(*p_fd);
    }

    // errno；
    // 由于我这个是脚本，不是真正编译程序；也就是说，从产生错误号，到获取
    // 错误号，间隔了多少系统调用？错误号是否被覆盖。
    return ::close(*p_fd) == -1 ? int64_t(errno) : int64_t(0);
}

REGIST_BUILTIN("read-line", 0, 1, eval_read_line,
               "; read-line 从stdin，或者文件描述符中读取一行数据，并返回字符串；如果遇到流结尾，返回nil\n"
               "(read-line) -> \"string\" | nil\n"
               "(read-line file-descriptor) -> \"string\" | nil");

/**
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_read_line(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "read-line";
    std::array<Object, 1> objs;
    int fd = 0; // 默认是stdin
    if (args.length()) {
        fd = int(*requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO));
    }

    std::string line;

    if (fd)
    {
        line = detail::readline(fd);
    }
    else
    {
        line = detail::readline_stdin();
    }
    if (line.empty() && errno) {
        return varlisp::Nill{};
    }
    return varlisp::string_t{std::move(line)};
}

REGIST_BUILTIN("read-char", 0, 1, eval_read_char,
               "(read-char) -> int64_t | nill\n"
               "(read-char file_descriptor) -> int64_t | nill");

/**
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_read_char(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "read-char";
    std::array<Object, 1> objs;

    int fd = 0;
    if (args.length()) {
        fd = int(*requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO));
    }

    int64_t ch = detail::readchar(fd);
    if (ch == -1 || errno) {
        return varlisp::Nill{};
    }

    return ch;
}

REGIST_BUILTIN("read-byte", 0, 1, eval_read_byte,
               "(read-byte) -> int64_t | nill\n"
               "(read-byte file_descriptor) -> int64_t | nill");

/**
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_read_byte(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "read-byte";
    std::array<Object, 1> objs;

    int fd = 0;
    if (args.length()) {
        fd = int(*requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO));
    }

    int64_t ch = detail::readbyte(fd);
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
    std::array<Object, 2> objs;
    const int64_t* p_fd =
        requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const int64_t* p_ch =
        requireTypedValue<int64_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    int64_t ec = detail::writechar(*p_fd, *p_ch);
    return (ec == -1) ? Object{varlisp::Nill{}} : Object{ec};
}

REGIST_BUILTIN("write-byte", 2, 2, eval_write_byte,
               "(write-byte file_descriptor int64_t) -> int64_t | nill");

/**
 * @brief (write-byte file_descriptor int64_t) -> int64_t | nill
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_write_byte(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "write-byte";
    std::array<Object, 2> objs;
    const int64_t* p_fd =
        requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const int64_t* p_ch =
        requireTypedValue<int64_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    int64_t ec = detail::writebyte(*p_fd, *p_ch);
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
    std::array<Object, 2> objs;
    const int64_t* p_fd =
        requireTypedValue<int64_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const varlisp::string_t* p_str =
        requireTypedValue<string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    int64_t ec = detail::writestring(*p_fd, p_str->to_string_view());
    return (ec == -1) ? Object{varlisp::Nill{}} : Object{ec};
}

REGIST_BUILTIN("get-fd-fname", 1, 1, eval_get_fd_fname,
               "(get-fd-fname fd) -> \"path\" | nill");

Object eval_get_fd_fname(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "get-fd-fname";
    Object obj;
    const int64_t* p_fd =
        requireTypedValue<int64_t>(env, args.nth(0), obj, funcName, 0, DEBUG_INFO);

    try {
        return string_t(detail::file::get_fname_from_fd(*p_fd));
    }
    catch (std::runtime_error& e) {
        COLOG_ERROR(e.what());
        return Nill{};
    }
}

// 交互
// 从文件描述符，读入一行数据。read(fd, buf, size)
// 如果从终端读入，默认行为是一行读取一次。
// 或者，因为signal，而终止读取，或者遇到了错误；
// 此时，有如下方式：
// 1. read(), byte-by-byte，然后判断是否换行符。
// 2. 利用gun-stl的非公开模式，将fd转化为std::istream来读取；
// 3. 按buffer读取——如果是终端，则默认读取到一行，或者超过了遇到signal，或者
//    超出了terminal行缓冲，或者超出了用户提供的buffer长度，都会返回。
//    当然，文件结尾，也会返回。
//    不过，上述方式，就无法处理从文件读取的行为了。
//! 从fd创建stream
//! http://stackoverflow.com/questions/2746168/how-to-construct-a-c-fstream-from-a-posix-file-descriptor
//
/// gnu-linux version
// int posix_handle = fileno(::fopen("test.txt", "r"));
//
// __gnu_cxx::stdio_filebuf<char> filebuf(posix_handle, std::ios::in); // 1
// istream is(&filebuf); // 2
//
// string line;
// getline(is, line);
// cout << "line: " << line << std::endl;
//
/// ms-vc version
// int posix_handle = ::_fileno(::fopen("test.txt", "r"));
//
// ifstream ifs(::_fdopen(posix_handle, "r")); // 1
//
// 更通用的方案，使用
// Another alternative would be to use a boost::iostreams::file_descriptor
// device, which you could wrap in a boost::iostreams::stream if you want to
// have an std::stream interface to it.
// #include <stdlib.h>
// #include <string.h>
// #include <assert.h>
// #include <string>
// #include <iostream>
// #include <boost/filesystem.hpp>
// #include <boost/iostreams/device/file_descriptor.hpp>
// #include <boost/iostreams/stream.hpp>
// 
// using boost::iostreams::stream;
// using boost::iostreams::file_descriptor_sink;
// using boost::filesystem::path;
// using boost::filesystem::exists;
// using boost::filesystem::status;
// using boost::filesystem::remove;
// 
// int main(int argc, const char *argv[]) {
//   char tmpTemplate[13];
//   strncpy(tmpTemplate, "/tmp/XXXXXX", 13);
//   stream<file_descriptor_sink> tmp(mkstemp(tmpTemplate));
//   assert(tmp.is_open());
//   tmp << "Hello mkstemp!" << std::endl;
//   tmp.close();
//   path tmpPath(tmpTemplate);
//   if (exists(status(tmpPath))) {
//     std::cout << "Output is in " << tmpPath.file_string() << std::endl;
//     std::string cmd("cat ");
//     cmd += tmpPath.file_string();
//     system(cmd.c_str());
//     std::cout << "Removing " << tmpPath.file_string() << std::endl;
//     remove(tmpPath);
//   }
// }
//
/// gnu version2
// #include <fstream>
// #include <string>
// #include <ext/stdio_filebuf.h>
// #include <type_traits>
// 
// bool OpenFileForSequentialInput(ifstream& ifs, const string& fname)
// {
//     ifs.open(fname.c_str(), ios::in);
//     if (! ifs.is_open()) {
//         return false;
//     }
// 
//     using FilebufType = __gnu_cxx::stdio_filebuf<std::ifstream::char_type>;
//     static_assert(  std::is_base_of<ifstream::__filebuf_type, FilebufType>::value &&
//                     (sizeof(FilebufType) == sizeof(ifstream::__filebuf_type)),
//             "The filebuf type appears to have extra data members, the cast might be unsafe");
// 
//     const int fd = static_cast<FilebufType*>(ifs.rdbuf())->fd();
//     assert(fd >= 0);
//     if (0 != posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL)) {
//         ifs.close();
//         return false;
//     }
// 
//     return true;
// }
/// stl 将fd封装为streambuf 的例子
//! http://stackoverflow.com/questions/13541313/handle-socket-descriptors-like-file-descriptor-fstream-c-linux
//
// std::stdio_filebuf<char> inbuf(fd, std::ios::in);
// std::istream istream(&inbuf);
// 
// std::stdio_filebuf<char> outbuf(fd, std::ios::out);
// std::ostream ostream(&outbuf);
//
// 另外，将fd封装后，有一个问题，那就是，我面向用户，提供的是fd接口，而内部读取就创建一个
// 自带buffer的stream，这显然是不行的。
//
// 除非我内部管理所有的fd，然后通过fd来检索，这样才能服用buffer，而不会发生错乱。
//

REGIST_BUILTIN("list-opened-fd", 0, 0, eval_list_opened_fd,
               "; list-opened-fd 枚举打开的文件描述符以及对应的文件名\n"
               "; 并以list的形式返回\n"
               "(list-opened-fd) -> [(fd name)...] | []");

Object eval_list_opened_fd(varlisp::Environment& env, const varlisp::List& args)
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
