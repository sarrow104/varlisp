#include <array>

#include <boost/process.hpp>
//#include <sss/linux/epollPipeRun.hpp>

#include <boost/process/pipe.hpp>

#include <chrono>
#include <iostream>
#include <sstream>

#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/environ.hpp>
#include <sss/path.hpp>
#include <sss/path/glob_path.hpp>
#include <sss/ps.hpp>
#include <sss/raw_print.hpp>
#include <sss/util/Escaper.hpp>

#include "../builtin_helper.hpp"
#include "../object.hpp"
#include "../raw_stream_visitor.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/list_iterator.hpp"
#include "../detail/varlisp_env.hpp"

namespace {

inline std::string dumpFromInStream(std::istream& iss) {
    std::ostringstream oss;
    oss << iss.rdbuf();
    return oss.str();
}

} // namespace

namespace varlisp {
// 应该如何处理shell-eval时候的参数？
// 比如，用户可能提供一个完整的命令行字符串——包括执行的命令，以及参数；
// 此时，如果画蛇添足地，增加引号，然后执行，就完蛋了。
//
// 同时，也有可能需要执行的命令，以及外部参数的参数，是分开的。比如，
// 其中一个参数，是代表路径的字符串，而且还有空格。那么，添加引号，
// 或者进行转义，就是一个必须的动作了。
//
// 但显然，这是两种不兼容的工作方式。
// 为此，特规定如下：

REGIST_BUILTIN("shell", 1, -1,  eval_shell,
               "(shell \"\") -> '(stdout, stderr)\n"
               "(shell \"\" arg1 arg2 arg3) -> '(stdout, stderr)");

/**
 * @brief
 *      (shell "") -> '(stdout, stderr)
 *      (shell "" arg1 arg2 arg3) -> '(stdout, stderr)
 *
 *  NOTE 不会对第一个参数进行转义！
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_shell(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "shell";
    std::ostringstream oss;

    std::array<Object, 1> objs;

    const auto* p_program =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    oss << *p_program->gen_shared();

    static sss::util::Escaper esp("\\ \"'[](){}?*$&");
    for (auto it = args.begin() + 1;
        it != args.end(); ++it)
    {
        Object tmp;
        const Object& current = getAtomicValue(env, *it, tmp);
        std::ostringstream inner_oss;
        boost::apply_visitor(raw_stream_visitor(inner_oss, env), current);
        std::string param = inner_oss.str();
        if (param.empty()) {
            continue;
        }
        oss << " ";
        esp.escapeToStream(oss, param);
    }

    COLOG_INFO("(", funcName, ": ", oss.str(), ')');

    namespace bp = boost::process;
	bp::ipstream pipe_out;
	bp::ipstream pipe_err;
	bp::system(oss.str(), bp::std_out > pipe_out, bp::std_err > pipe_err);

    std::string out = dumpFromInStream(pipe_out);
    std::string err = dumpFromInStream(pipe_err);
	//pipe_out >> out;
	//pipe_err >> err;

    varlisp::List ret = varlisp::List::makeSQuoteList(
        string_t(out), string_t(err));

    return ret;
}

REGIST_BUILTIN("shell-pipe", 1, -1,  eval_shell_pipe,
               "(shell-pipe \"cmd [arg1...]\") -> '(stdout, stderr)\n"
               "(shell-pipe \"cmd [arg1...]\" stdin...) -> '(stdout, stderr)");

/**
 * @brief
 *      (shell-pipe "cmd [arg1...]") -> '(stdout, stderr)
 *      (shell-pipe "cmd [arg1...]" stdin...) -> '(stdout, stderr)
 *
 *  NOTE 不会对第一个参数进行转义！
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_shell_pipe(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "shell-pipe";

    std::array<Object, 1> objs;

    const auto* p_program =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::string cmd = *p_program->gen_shared();

    COLOG_INFO("(", funcName, ": ", cmd, ')');

    namespace bp = boost::process;
    bp::opstream pipe_ops;
	bp::ipstream pipe_out;
	bp::ipstream pipe_err;

    // https://stackoverflow.com/questions/65860643/passing-file-or-stdin-to-boost-process-child-ambiguously
    bp::child c(cmd, bp::std_in < pipe_ops, bp::std_out > pipe_out, bp::std_err > pipe_err);
    // https://kheresy.wordpress.com/2016/10/07/boost-process/

    for (auto it = args.begin() + 1;
        it != args.end(); ++it)
    {
        Object tmp;
        const Object& current = getAtomicValue(env, *it, tmp);
        std::ostringstream inner_oss;
        boost::apply_visitor(raw_stream_visitor(inner_oss, env), current);
        std::string param = inner_oss.str();
        if (param.empty()) {
            continue;
        }
        COLOG_INFO("(", funcName, " send ", param.size(), " bytes)");
        pipe_ops << param;
    }
    COLOG_INFO("(", funcName, " close input stream)");
    pipe_ops.flush();
    pipe_ops.pipe().close();
    // https://stackoverflow.com/questions/48657409/how-to-redirect-stdin-and-stdout-using-boost-process
    // c.wait();
    c.wait_for(std::chrono::seconds{1});

    std::string out = dumpFromInStream(pipe_out);
    std::string err = dumpFromInStream(pipe_err);
	//pipe_out >> out;
	//pipe_err >> err;

    varlisp::List ret = varlisp::List::makeSQuoteList(
        string_t(out), string_t(err));

    return ret;
}

REGIST_BUILTIN("system", 1, -1,  eval_system,
               "(system \"cmd\") -> return-code\n"
               "(system \"cmd\" arg1 arg2 arg3) -> return-code");

/**
 * @brief
 *
 *  NOTE 不会对第一个参数进行转义！
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */

Object eval_system(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "system";

    std::array<Object, 1> objs;

    const auto* p_program =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::ostringstream oss;
    oss << *p_program->gen_shared();

    static sss::util::Escaper esp("\\ \"'[](){}?*$&");
    for (auto it = args.begin() + 1;
        it != args.end(); ++it)
    {
        Object tmp;
        const Object& current = getAtomicValue(env, *it, tmp);
        std::ostringstream inner_oss;
        boost::apply_visitor(raw_stream_visitor(inner_oss, env), current);
        std::string param = inner_oss.str();
        if (param.empty()) {
            continue;
        }
        oss << " ";
        esp.escapeToStream(oss, param);
    }

    std::string out;
    std::string err;

    COLOG_INFO("(", funcName, ": ", oss.str(), ')');

    return {static_cast<int64_t>(std::system(oss.str().c_str()))};
}

REGIST_BUILTIN("shell-cd", 1, 1, eval_shell_cd,
               "(shell-cd \"path/to/go\") -> \"new-work-dir\"");

/**
 * @brief (shell-cd "path/to/go") -> "new-work-dir"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_shell_cd(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "shell-cd";
    std::array<Object, 1> objs;
    const auto* p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::string target_path = sss::path::full_of_copy(*p_path->gen_shared());

    bool is_ok = sss::path::chgcwd(target_path);
    COLOG_INFO("(", funcName, ": ", sss::raw_string(*p_path),
               is_ok ? "succeed" : "failed", ")");
    return {string_t{sss::path::getcwd()}};
}

REGIST_BUILTIN("shell-mkdir", 1, 1, eval_shell_mkdir,
               "(shell-mkdir \"path/to/make\") -> \"full/path\" | nil");

Object eval_shell_mkdir(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "shell-mkdir";
    std::array<Object, 1> objs;
    const auto* p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    bool is_ok = sss::path::mkpath(*p_path->gen_shared());
    COLOG_INFO("(", funcName, ": ", sss::raw_string(*p_path),
               is_ok ? "succeed" : "failed", ")");
    if (is_ok) {
        return {*p_path};
    }
    return Nill{};
}

REGIST_BUILTIN("shell-ls", 0, -1, eval_shell_ls,
               "(shell-ls \"dir1\" \"dir2\" ...) -> '(\"item1\",\"item2\", ...)");

// 允许任意个可以理解为路径的字符串作为参数；枚举出所有路径
/**
 * @brief
 *     (shell-ls "dir1" "dir2" ...) -> '("item1","item2", ...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_shell_ls(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "shell-ls";
    varlisp::List ret = varlisp::List::makeSQuoteList();
    auto ret_it = detail::list_back_inserter<Object>(ret);
    if (args.length() != 0U) {

        for (size_t i = 0; i < args.length(); ++i) {
            std::array<Object, 1> objs;
            const auto* p_ls_arg =
                requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

            std::string ls_path = sss::path::full_of_copy(varlisp::detail::envmgr::expand(*p_ls_arg->gen_shared()));
            switch (sss::path::file_exists(ls_path)) {
                case sss::PATH_TO_FILE:
                    *ret_it++ = *p_ls_arg;
                    break;

                case sss::PATH_TO_DIRECTORY: {
                    sss::path::file_descriptor fd;
                    sss::path::glob_path gp(ls_path, fd);
                    while (gp.fetch()) {
                        if (fd.is_normal_dir()) {
                            std::string item{fd.get_name()};
                            item += sss::path::sp_char;
                            *ret_it++ = string_t(item);
                        }
                        else if (fd.is_normal_file()) {
                            *ret_it++ = string_t(fd.get_name());
                        }
                    }
                } break;

                case sss::PATH_NOT_EXIST:
                    COLOG_ERROR("(", funcName, ": path", sss::raw_string(*p_ls_arg),
                                "not exists)");
                    break;
            }
        }
    }
    else {
        // NOTE
        // 没提供参数的时候，貌似失败；Object默认值为nil，导致0长度的args，不存在！
        // 看样子，必须要使用头结点！或者，额外再增加一个Nil类才行！
        sss::path::file_descriptor fd;
        sss::path::glob_path gp(".", fd);
        while (gp.fetch()) {
            if (fd.is_normal_dir()) {
                std::string item{fd.get_name()};
                item += sss::path::sp_char;
                *ret_it++ = string_t(item);
            }
            else if (fd.is_normal_file()) {
                *ret_it++ = string_t(fd.get_name());
            }
        }
    }
    return ret;
}

REGIST_BUILTIN("shell-pwd", 0, 0, eval_shell_pwd,
               "(shell-pwd) -> \"current-working-dir\"");

/**
 * @brief (pwd) -> "current-working-dir"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_shell_pwd(varlisp::Environment& env, const varlisp::List& args)
{
    (void)env;
    (void)args;
    return {string_t(sss::path::getcwd())};
}

REGIST_BUILTIN("shell-env", 0, 1, eval_shell_env,
               "(shell-env) -> {(kev \"value\")...}\n"
               "(shell-env key) -> \"value-string\" | nil");

Object eval_shell_env(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "shell-env";
    if (args.empty()) {
        varlisp::Environment sysEnv(&env);

        char ** p_env = environ;
        while ((p_env != nullptr) && (p_env[0] != nullptr)) {
            int eq_pos = std::strchr(p_env[0], '=') - p_env[0];
            sysEnv[std::string(p_env[0], eq_pos)] = string_t(std::string(p_env[0] + eq_pos + 1));
            p_env++;
        }

        return sysEnv;
    }
    std::array<Object, 1> objs;
    const auto* p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    const char * p_value = ::getenv(p_path->gen_shared()->c_str());
    if (p_value == nullptr) {
        return Nill{};
    }
    return string_t(std::string(p_value));
}

}  // namespace varlisp
