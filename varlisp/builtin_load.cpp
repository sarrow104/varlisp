#include <stdexcept>

#include <sss/colorlog.hpp>
#include <sss/path.hpp>
#include <sss/raw_print.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "interpreter.hpp"

#include "detail/buitin_info_t.hpp"
#include "detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("load", 1, 1, eval_load, "(load \"path/to/lisp\") -> nil");

/**
 * @brief (load "path/to/lisp") -> nil
 *
 * TODO
 * 由于载入之后的脚本，马上就被执行了——相当于从终端输入。那么，
 * 从逻辑上，返回值应该是脚本中最后一条语句的值。
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_load(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "load";
    // NOTE 既然
    // load是内建函数，那么load，就可以出现在任何地方；比如另外一个脚本；
    // 而，我的解释器，对于脚本的load，本质上，是让内部的Tokenizer对象，
    // push、pop一个额外的工作栈——但是，当前，只是，Tokenizer只拥有两个工作栈，相当于双缓冲区；
    // 对于嵌套load的使用，这可能会力不从心！
    //
    // 于是，安全的作法有两种：
    //      1. 让Tokenizer对象，成为真正的多栈的对象；
    //      2.
    //      按需要，临时构建Tokenizer对象，以供解析用——相当于有了Tokenizer的堆栈；
    //
    // NOTE 另外，在load的时候，是否需要新建一个Environment对象呢？
    // 这是作用域的问题；即，对于(load "path/to/script")语句而言，新解析到
    // 的标识符(对象、函数等等)，应该放到哪个Environment中呢？

    Object path;
    const string_t* p_path =
        getTypedValue<string_t>(env, detail::car(args), path);
    if (!p_path) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requies a path)");
    }
    std::string full_path = sss::path::full_of_copy(p_path->to_string());
    if (sss::path::file_exists(full_path) != sss::PATH_TO_FILE) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName, "`", *p_path,
                          "` not to file)");
    }

    std::string content;
    sss::path::file2string(full_path, content);

    varlisp::Interpreter* p_inter = env.getInterpreter();
    if (!p_inter) {
        SSS_POSITION_THROW(std::runtime_error,
                          "env.getInterpreter return 0 ptr");
    }
    varlisp::Parser& parser = p_inter->get_parser();
    parser.parse(env, content, true);
    COLOG_INFO("(", funcName, sss::raw_string(*p_path), " complete)");
    return Object{Nill{}};
}

}  // namespace varlisp
