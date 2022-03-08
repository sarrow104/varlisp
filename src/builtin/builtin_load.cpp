#include <stdexcept>
#include <set>
#include <list>

#include <sss/colorlog.hpp>
#include <sss/path.hpp>
#include <sss/raw_print.hpp>
#include <sss/penvmgr2.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../interpreter.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/json_accessor.hpp"
#include "../detail/varlisp_env.hpp"

namespace varlisp {

namespace detail {
typedef std::list<varlisp::string_t> script_stack_t;
script_stack_t& get_script_stack()
{
    static script_stack_t script_stack;
    return script_stack;
}

struct load_guard_t
{
    script_stack_t &m_st;
    explicit load_guard_t(script_stack_t& st, const varlisp::string_t& path)
        : m_st(st)
    {
        // COLOG_ERROR(path);
        st.push_back(path);
    }
    ~load_guard_t()
    {
        // COLOG_ERROR(m_st.back());
        m_st.pop_back();
    }
};

} // namespace detail

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

    const char* funcName = "load";
    std::array<Object, 1> objs;
    const auto* p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    auto full_path = varlisp::detail::envmgr::expand(*p_path->gen_shared());
    if (sss::path::is_relative(full_path) && !detail::get_script_stack().empty()) {
        full_path =
            sss::path::append_copy(sss::path::dirname(*detail::get_script_stack().back().gen_shared()), *p_path->gen_shared());
    }
    else {
        full_path = sss::path::full_of_copy(full_path);
    }

    if (sss::path::file_exists(full_path) != sss::PATH_TO_FILE && sss::path::suffix(full_path) != ".lsp") {
        full_path += ".lsp";
    }

    if (sss::path::file_exists(full_path) != sss::PATH_TO_FILE) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName, "`", *p_path,
                          "` not to file)");
    }

    std::string content;
    sss::path::file2string(full_path, content);
    detail::load_guard_t guard(detail::get_script_stack(), varlisp::string_t(full_path));
    try {
        // NOTE FIXME 我这里的困境在于，我都是从一个地方，获取的parser实例。而parser在内部，完成的eval（传入了env，content）
        // 同一个stack。这就导致了，内部load的时候，实际也是往一个stack里面加东西——嵌套。
        // 所以，最终的 detail::load_guard_t 执行结果，不如人意。
        varlisp::Parser& parser = varlisp::Interpreter::get_instance().get_parser();
        parser.parse(env, content, true);
        COLOG_INFO("(", funcName, sss::raw_string(*p_path), " complete)");
        return Object{Nill{}};
    }
    catch (...) {
        throw;
    }
}

REGIST_BUILTIN("save", 1, 1, eval_save,
               "(save \"path/to/lisp\") -> item-count");

Object eval_save(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "load";

    std::array<Object, 1> objs;
    const auto* p_path =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::string full_path = sss::path::full_of_copy(*p_path->gen_shared());
    if (sss::path::file_exists(full_path) == sss::PATH_TO_DIRECTORY) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName, "`", *p_path,
                          "` is a dir! cannot write context into a dir!)");
    }

    sss::path::mkpath(sss::path::dirname(full_path));
    std::ofstream ofs(full_path.c_str(), std::ios_base::out | std::ios_base::app);
    std::set<std::string> dumped_obj_set;
    int64_t item_cnt = 0;
    for (const varlisp::Environment * p_env = &env; p_env != nullptr; p_env = p_env->parent()) {
        for (const auto & it : *p_env) {
            if (dumped_obj_set.find(it.first) == dumped_obj_set.end()) {
                dumped_obj_set.insert(it.first);
                if (boost::get<varlisp::Builtin>(&it.second.first) != nullptr) {
                    continue;
                }
                if (it.second.second.is_const) {
                    continue;
                }
                ofs << "(define " << it.first << " ";
                boost::apply_visitor(print_visitor(ofs), it.second.first);
                ofs << ")" << std::endl;
                ++item_cnt;
            }
        }
    }
    return item_cnt;
}

// NOTE FIXME 完蛋！
// (script) 没用！
// 因为载入与运行时，不是一块！
// (load)
REGIST_BUILTIN("script", 0, 1, eval_script,
               "; script 脚本状态\n"
               "(script) -> 当前脚本\n"
               "(script index) -> 父级脚本");

Object eval_script(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "script";
    int64_t index = 0;
    if (!args.empty()) {
        Object tmp;
        index =
            *requireTypedValue<int64_t>(env, args.nth(0), tmp, funcName, 0, DEBUG_INFO);
    }
    auto& st = detail::get_script_stack();
    auto it = st.rbegin();
    Object ret = Nill{};
    for (auto i = index; i >= 0 && it != st.rend(); --i, ++it) {
        if (i == 0) {
            ret = *it;
        }
    }
    return ret;
}

REGIST_BUILTIN("script-depth", 0, 0, eval_depth,
               "; script-depth 脚本状态\n"
               "(script-depth) -> int64_t");

Object eval_depth(varlisp::Environment&  /*env*/, const varlisp::List&  /*args*/)
{
    return int64_t(detail::get_script_stack().size());
}

REGIST_BUILTIN("clear", 0, 1, eval_clear,
               "(clear) -> item-count");

Object eval_clear(varlisp::Environment& env, const varlisp::List& args)
{
    auto * p_target = &env;
    const char * funcName = "clear";
    Object tmp;
    if (args.length() != 0U) {
        const auto * p_sym = varlisp::getSymbol(env, detail::car(args), tmp);
        if (p_sym == nullptr) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": 1st arg must be sym)");
        }

        auto jc = detail::json_accessor{p_sym->name()};
        auto location = jc.locate(env);
        if (location.env == nullptr) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": sym ", *p_sym, " not exist)");
        }
        p_target = boost::get<varlisp::Environment>(const_cast<Object*>(location.obj));
        if (p_target == nullptr) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": ", *p_sym, " is not an env{})");
        }
    }
    return static_cast<int64_t>(p_target->clear());
}

}  // namespace varlisp
