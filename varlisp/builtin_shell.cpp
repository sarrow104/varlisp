#include "eval_visitor.hpp"
#include "object.hpp"
#include "raw_stream_visitor.hpp"

#include <sss/ps.hpp>
#include <sss/linux/epollPipeRun.hpp>
#include <sss/util/Escaper.hpp>

#include <sss/colorlog.hpp>
#include <sss/path.hpp>
#include <sss/path/glob_path.hpp>
#include <sss/raw_print.hpp>

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
    std::ostringstream oss;
    auto & type = args.head.type();
    std::string program;

    if (type == typeid(std::string)) {
        program = *boost::get<std::string>(&args.head);
    }
    else if (type == typeid(varlisp::symbol) || type == typeid(varlisp::List)) {
        Object result = boost::apply_visitor(eval_visitor(env), args.head);
        if (result.type() == typeid(std::string)) {
            program = *boost::get<std::string>(&result);
        }
    }
    if (program.empty()) {
        SSS_POSTION_THROW(std::runtime_error, "(shell: 1st arg must be an none-empty string)");
    }
    oss << program;
    static sss::util::Escaper esp("\\ \"'[](){}?*$&");
    for (const varlisp::List * p_list = args.next(); p_list && p_list->head.which(); p_list = p_list->next()) {
        std::ostringstream inner_oss;
        boost::apply_visitor(raw_stream_visitor(inner_oss, env), p_list->head);
        std::string param = inner_oss.str();
        if (param.empty()) {
            continue;
        }
        oss << " ";
        esp.escapeToStream(oss, param);
    }

    std::string out, err;
    
    COLOG_INFO("(shell run: ", oss.str(), ')');
    std::tie(out, err) = sss::epoll::rwe_pipe_run(oss.str(), 1 + 2);

    varlisp::List ret = varlisp::List::makeSQuoteList();
    varlisp::List * p_list = &ret;
    p_list = p_list->next_slot();
    p_list->head = out;
    p_list = p_list->next_slot();
    p_list->head = err;

    return Object(ret);
}

/**
 * @brief (cd "path/to/go") -> "new-work-dir"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_cd(varlisp::Environment& env, const varlisp::List& args)
{
    Object target_path = boost::apply_visitor(eval_visitor(env), args.head);
    const std::string* p_path = boost::get<std::string>(&target_path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error,
                          "shell-cd: requie one path string!");
    }
    sss::path::chgcwd(*p_path);
    COLOG_INFO("(shell-cd: ", sss::raw_string(*p_path), " complete)");
    return Object(sss::path::getcwd());
}

// {"ls",          0, -1,  &eval_ls},      //
// 允许任意个可以理解为路径的字符串作为参数；枚举出所有路径
/**
 * @brief (ls "dir1" "dir2" ...) -> '("item1","item2", ...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_ls(varlisp::Environment& env, const varlisp::List& args)
{
    varlisp::List ret = varlisp::List::makeSQuoteList();
    const List* p = &args;
    List* p_list = &ret;
    if (args.length() && args.head.which()) {
        while (p && p->head.which()) {
            Object ls_arg = boost::apply_visitor(eval_visitor(env), p->head);
            const std::string* p_ls_arg = boost::get<std::string>(&ls_arg);
            if (!p_ls_arg) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "shell-ls: require string-type args");
            }
            switch (sss::path::file_exists(*p_ls_arg)) {
                case sss::PATH_TO_FILE:
                    p_list = p_list->next_slot();
                    p_list->head = *p_ls_arg;
                    break;

                case sss::PATH_TO_DIRECTORY: {
                    sss::path::file_descriptor fd;
                    sss::path::glob_path gp(*p_ls_arg, fd);
                    while (gp.fetch()) {
                        if (fd.is_normal_dir()) {
                            p_list = p_list->next_slot();
                            p_list->head =
                                std::string(fd.get_name()) + sss::path::sp_char;
                        }
                        else if (fd.is_normal_file()) {
                            p_list = p_list->next_slot();
                            p_list->head = std::string(fd.get_name());
                        }
                    }
                } break;

                case sss::PATH_NOT_EXIST:
                    COLOG_ERROR("(shell-ls: path", sss::raw_string(*p_ls_arg),
                                "not exists)");
                    break;
            }
            p = p->next();
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
                p_list = p_list->next_slot();
                p_list->head = std::string(fd.get_name()) + sss::path::sp_char;
            }
            else if (fd.is_normal_file()) {
                p_list = p_list->next_slot();
                p_list->head = std::string(fd.get_name());
            }
        }
    }
    return Object(ret);
}

/**
 * @brief (pwd) -> "current-working-dir"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_pwd(varlisp::Environment& env, const varlisp::List& args)
{
    (void)env;
    (void)args;
    return Object(sss::path::getcwd());
}

}  // namespace varlisp
