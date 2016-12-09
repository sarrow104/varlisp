#include <sss/linux/epollPipeRun.hpp>
#include <sss/ps.hpp>
#include <sss/util/Escaper.hpp>
#include <sss/colorlog.hpp>
#include <sss/path.hpp>
#include <sss/path/glob_path.hpp>
#include <sss/raw_print.hpp>

#include "object.hpp"
#include "raw_stream_visitor.hpp"
#include "builtin_helper.hpp"

#include "detail/buitin_info_t.hpp"
#include "detail/list_iterator.hpp"
#include "detail/car.hpp"

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
               "(shell "") -> '(stdout, stderr)\n"
               "(shell "" arg1 arg2 arg3) -> '(stdout, stderr)");

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
// TODO 我想增加一组"强化"的boost::get<type>函数，以便支持单向的提取——允许eval；
// 如果eval后，返回值类型不对，那么抛出异常；
Object eval_shell(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "shell";
    std::ostringstream oss;

    Object program;

    const string_t* p_program =
        getTypedValue<string_t>(env, detail::car(args), program);
    if (!p_program || p_program->empty()) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": 1st arg must be an none-empty string)");
    }

    oss << p_program->to_string();

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

    std::string out, err;

    COLOG_INFO("(", funcName, ": ", oss.str(), ')');
    std::tie(out, err) = sss::epoll::rwe_pipe_run(oss.str(), 1 + 2);

    varlisp::List ret = varlisp::List::makeSQuoteList();
    auto ret_it = detail::list_back_inserter<Object>(ret);

    *ret_it++ = string_t(std::move(out));
    *ret_it = string_t(std::move(err));

    return Object(ret);
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
    Object path;
    const string_t* p_path =
        getTypedValue<string_t>(env, detail::car(args), path);
    if (!p_path) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requie one path string!)");
    }
    bool is_ok = sss::path::chgcwd(p_path->to_string());
    COLOG_INFO("(", funcName, ": ", sss::raw_string(*p_path),
               is_ok ? "succeed" : "failed", ")");
    return Object(string_t{std::move(sss::path::getcwd())});
}

REGIST_BUILTIN("shell-ls", 0, -1, eval_shell_ls,
               "(ls \"dir1\" \"dir2\" ...) -> '(\"item1\",\"item2\", ...)");

// 允许任意个可以理解为路径的字符串作为参数；枚举出所有路径
/**
 * @brief (ls "dir1" "dir2" ...) -> '("item1","item2", ...)
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
    if (args.length() && detail::car(args).which()) {
        
        for (auto it = args.begin(); it != args.end(); ++it) {
            Object ls_arg;
            const string_t* p_ls_arg = getTypedValue<string_t>(env, *it, ls_arg);
            if (!p_ls_arg) {
                SSS_POSITION_THROW(std::runtime_error,
                                  "(", funcName, ": require string-type args)");
            }
            switch (sss::path::file_exists(p_ls_arg->to_string())) {
                case sss::PATH_TO_FILE:
                    *ret_it++ = *p_ls_arg;
                    break;

                case sss::PATH_TO_DIRECTORY: {
                    sss::path::file_descriptor fd;
                    sss::path::glob_path gp(p_ls_arg->to_string(), fd);
                    while (gp.fetch()) {
                        if (fd.is_normal_dir()) {
                            std::string item{fd.get_name()};
                            item += sss::path::sp_char;
                            *ret_it++ = string_t(std::move(item));
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
                *ret_it++ = string_t(std::move(item));
            }
            else if (fd.is_normal_file()) {
                *ret_it++ = string_t(fd.get_name());
            }
        }
    }
    return Object(ret);
}

REGIST_BUILTIN("shell-pwd", 0, 0, eval_shell_pwd,
               "(pwd) -> \"current-working-dir\"");

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
    return Object(string_t(std::move(sss::path::getcwd())));
}

}  // namespace varlisp
