#include <set>
#include <array>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../environment.hpp"
#include "../cast_visitor.hpp"
#include "../eval_visitor.hpp"
#include "../keyword_t.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../tokenizer.hpp"
#include "../keyword_t.hpp"
#include "../detail/car.hpp"
#include "../detail/json_accessor.hpp"
#include "../detail/list_iterator.hpp"

namespace varlisp {

REGIST_BUILTIN("undef", 1, 1, eval_undef, "(undef symbol) -> boolean");

/**
 * @brief
 *      (undef symbol) -> boolean
 *      删除变量
 *      Builtin不允许删除
 *      但是，你可以在内层函数中，定义自己的操作，以修改定义；
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_undef(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "undef";
    const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&detail::car(args));
    if (!p_sym) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a symbol)");
    }
    if (varlisp::keywords_t::is_keyword(p_sym->name())) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": cannot undef keywords", p_sym->name(), ")");
    }
    detail::json_accessor jc{p_sym->name()};
    bool ret = false;
    auto location = detail::json_accessor::locate(env, *p_sym);
    if (!location.first) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": tobe delete symbol ", p_sym->name(), " not exist)");
    }
    if (jc.has_sub()) {
        if (!sss::is_all(jc.stems().back(), static_cast<int(*)(int)>(std::isdigit))) {
            location.second->erase(jc.stems().back());
            ret = true;
        }
        else {
            std::vector<int> indexer;
            for (size_t i = jc.stems().size(); i != 0; --i) {
                if (!sss::is_all(jc.stems()[i - 1], static_cast<int(*)(int)>(std::isdigit))) {
                    break;
                }
                indexer.push_back(sss::string_cast<int>(jc.stems()[i - 1]));
            }
            std::reverse(indexer.begin(), indexer.end());
            COLOG_ERROR(indexer);
            // TODO
        }
    }
    else {
        location.second->erase(p_sym->name());
        ret = true;
    }

    return ret;
}

REGIST_BUILTIN("ifdef", 1, 1, eval_ifdef, "(ifdef symbol) -> boolean");

/**
 * @brief
 *      (ifdef symbol) -> boolean
 *      测试是否定义了某变量
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_ifdef(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "ifdef";
    const varlisp::symbol * p_sym = boost::get<varlisp::symbol>(&detail::car(args));
    if (!p_sym) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a symbol)");
    }
    if (varlisp::keywords_t::is_keyword(p_sym->name())) {
        return true;
    }
    Object* it = env.deep_find(p_sym->name());
    return bool(it);
}

REGIST_BUILTIN("var-list", 0, 1, eval_var_list,
               "(var-list) -> int64_t\n"
               "(var-list env-name) -> int64_t");

/**
 * @brief
 *      (var-list) -> int64_t
 *      枚举所有变量；并返回对象计数
 *      (var-list env-name) -> int64_t
 *      枚举提供的环境名所拥有的变量；并返回对象计数
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_var_list(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "ifdef";
    const varlisp::Environment * p_env = &env;
    int64_t var_count = 0;
    std::array<Object, 1> objs;
    if (args.length()) {
       p_env =
            varlisp::requireTypedValue<varlisp::Environment>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    }

    // 被子环境覆盖了的父环境变量，不会输出。
    // keyword不允许覆盖定义
    std::set<std::string> outted;
    for (; p_env; p_env = p_env->parent()) {
        for (auto it = p_env->begin(); it != p_env->end(); ++it) {
            if (outted.find(it->first) == outted.end()) {
                std::cout << it->first << "\n"
                    << "\t" << it->second.first << (it->second.second.is_const ? " CONST" : "")
                    << std::endl;
                outted.insert(it->first);
            }
        }
    }
    var_count = outted.size();
    return var_count;
}

namespace detail {
Object let_and_letn_impl(varlisp::Environment& env, const varlisp::List& args,
                         const char* funcName, bool reuse_inner)
{
    const varlisp::List * p_sym_pairs = boost::get<varlisp::List>(&detail::car(args));
    COLOG_DEBUG(detail::car(args));
    if (!p_sym_pairs) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a list; but",
                           detail::car(args), ")");
    }
    varlisp::Environment inner(&env);
    for (auto it_pair = p_sym_pairs->begin(); it_pair != p_sym_pairs->end(); ++it_pair) {
        COLOG_DEBUG(*it_pair);
        const varlisp::List* p_sym_pair =
            boost::get<varlisp::List>(&(*it_pair));
        if (!p_sym_pair) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": must be name-value  list; but",
                               p_sym_pairs->nth(0), ")");
        }
        if (p_sym_pair->length() != 2) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": must be name-value  list; but",
                               p_sym_pairs->nth(0), ")");
        }
        const varlisp::symbol* p_sym =
            boost::get<varlisp::symbol>(&p_sym_pair->nth(0));

        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": must be name-value  list; but",
                               *it_pair, ")");
        }
        Object value;
        // NOTE let 貌似可以达到swap两个变量值的效果；
        // 在于 getAtomicValue()的第一个参数是env，而不是inner
        // (let ((a b) (b a)) ...)
        // 而letn和let的区别，仅在于getAtomicValue的第一个参数，是inner，还是
        // env;
        inner[p_sym->name()] = varlisp::getAtomicValue(reuse_inner ? inner : env,
                                                       p_sym_pair->nth(1),
                                                       value);
    }
    Object result;
    for (auto it_expr = args.begin() + 1; it_expr != args.end(); ++it_expr) {
        result = boost::apply_visitor(eval_visitor(inner), *it_expr);
    }
    
    return result;
}

} // namespace detail

REGIST_BUILTIN("let", 2, -1, eval_let,
               "(let ((symbol expr)...) (expr)...) -> result-of-last-expr");

/**
 * @brief
 *      (let ((symbol expr)...)
 *           (expr)...) -> result-of-last-expr
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_let(varlisp::Environment& env, const varlisp::List& args)
{
    return detail::let_and_letn_impl(env, args, "let", false);
}

REGIST_BUILTIN("letn", 2, -1, eval_letn,
               "letn 可重用版let；后面定义的符号，可以使用前面符号的值\n"
               "(letn ((symbol expr)...) (expr)...) -> result-of-last-expr\n"

               "(letn\n"
               "   (x 2\n"
               "    y (pow x 3)\n"
               "    z (pow x 4))\n"
               " (println x)\n"
               " (println y)\n"
               " (println z))\n"
              );

Object eval_letn(varlisp::Environment& env, const varlisp::List& args)
{
    return detail::let_and_letn_impl(env, args, "letn", true);
}

REGIST_BUILTIN("set", 2, 2, eval_set,
               "; 对符号引用到的变量做修改；如果变量不存在，则报错\n"
               "(set 'quote-symbal expr) -> value-of-expr");

Object eval_set(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "setq";
    std::array<Object, 2> objs;
    auto * p_quoted_symbol =
        varlisp::getTypedValue<varlisp::List>(env, args.nth(0), objs[0]);
    if (p_quoted_symbol && p_quoted_symbol->is_quoted()) {
        if(auto * p_sym = boost::get<varlisp::symbol>(p_quoted_symbol->unquote())) {
            if (auto * p_value = env.deep_find(p_sym->name())) {
                *p_value = varlisp::getAtomicValue(env, args.nth(1), objs[1]);
                return *p_value;
            }
            else {
                SSS_POSITION_THROW(std::runtime_error, "(", funcName, ": symbol, ",
                                   *p_sym, " not exist!)");
            }
        }
        else {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": need a quoted symbol, as 1st argument, but ",
                               *p_quoted_symbol, " )");
        }
    }
    else {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": need a quoted symbol, as 1st argument, but ",
                               args.nth(0), " )");
    }
}

REGIST_BUILTIN("setq", 2, -1, eval_setq,
               "; 对符号引用到的变量做修改；如果变量不存在，则报错\n"
               "((setq symbol1 expr1 symbol2 expr2 ... ) -> value-of-last-expr");

/**
 * @brief
 *      (setq symbol1 expr1 symbol2 expr2 ... ) -> value-of-last-expr
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_setq(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "setq";
    if (args.size() % 2 != 0) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": num of args not correct!", args.size(), ")");
    }
    Object * p_value = 0;
    for (size_t i = 0; i < args.size(); i += 2) {
        // NOTE 这里需要的是一个将 Object cast 为 symbol的函数
        Object tmpObj;
        const varlisp::symbol * p_sym = varlisp::getSymbol(env,
                                                           args.nth(i),
                                                           tmpObj);
        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": 1st must be a symbol; but ", args.nth(i),
                               ")");
        }
        p_value = env.deep_find(p_sym->name());
        if (!p_value) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName, ": symbol, ",
                               *p_sym, " not exist!)");
        }
        Object res;
        *p_value = varlisp::getAtomicValue(env, args.nth(i + 1), res);
    }
    if (!p_value) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": last value nullptr error!)");
    }
    return *p_value;
}

REGIST_BUILTIN("setf", 2, 2, eval_setf,
               "; setf 是使用了setq的宏；暂时不支持！\n"
               "(setf \"varname\" expr) -> nil");

/**
 * @brief
 *      (setf "varname" expr) -> nil
 *
 * http://www.lispworks.com/documentation/HyperSpec/Body/m_setf_.htm
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_setf(varlisp::Environment& env, const varlisp::List& args)
{
    // TODO FIXME
    const char * funcName = "setf";
    SSS_POSITION_THROW(std::runtime_error,
                       "(",funcName,": not support yet!)");

    return varlisp::Nill{};
}

REGIST_BUILTIN("swap", 2, 2, eval_swap,
               "; swap 交换两个变量的值；变量查找办法同setq\n"
               "(swap var1 var2) -> nil");

/**
 * @brief
 *      (swap var1 var2) -> nil
 *
 * http://www.lispworks.com/documentation/HyperSpec/Body/m_setf_.htm
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_swap(varlisp::Environment& env, const varlisp::List& args)
{
    // TODO FIXME
    const char * funcName = "swap";
    const varlisp::symbol * p_sym1 = boost::get<varlisp::symbol>(&detail::car(args));
    if (!p_sym1) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st must be a symbol)");
    }

    const varlisp::symbol * p_sym2 = boost::get<varlisp::symbol>(&detail::cadr(args));
    if (!p_sym2) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 2nd must be a symbol)");
    }

    // FIXME swap也需要重建 binding
    Object * p_val1 = env.deep_find(p_sym1->name());
    if (!p_val1) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 1st symbol, ", *p_sym1, " not exist)");
    }
    Object * p_val2 = env.deep_find(p_sym2->name());
    if (!p_val2) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 2nd symbol, ", *p_sym2, " not exist)");
    }

    std::swap(*p_val1, *p_val2);
    return varlisp::Nill{};
}

REGIST_BUILTIN("locate", 2, 2, eval_locate,
               "; locate 在某{}或者[]，按照json-accessor的语法，返回子对象的值\n"
               "; 允许字面值\n"
               "(locate {}-or-[]-value '(symbol-or-int...)) -> sub-value");

// TODO 如何用类似的语法，进行更改值呢？
Object eval_locate(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "locate";
    Object tmp;
    const Object& objRef = varlisp::getAtomicValue(env, detail::car(args), tmp);

    Object tmpStems;
    auto p_stem_list = varlisp::getQuotedList(env, detail::cadr(args), tmpStems);
    if (!p_stem_list) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 2nd argument must be a s-list; but ", detail::cadr(args), " )");
    }

    auto * p_obj = &objRef;
    auto * p_env = &env;
    for (auto stem_it = p_stem_list->begin(); stem_it != p_stem_list->end(); ++stem_it) {
        if (auto * p_index = boost::get<int64_t>(&*stem_it)) {
            auto * p_list = boost::get<varlisp::List>(p_obj);
            if (!p_list) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "need a List here , but ", p_obj->which());
            }
            p_list = p_list->unquoteType<varlisp::List>();
            if (!p_list) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "need a s-List here , but ", p_obj->which());
            }

            p_obj = const_cast<varlisp::Object*>(&p_list->nth(*p_index));
            if (!p_obj) {
                SSS_POSITION_THROW(std::runtime_error,
                                   *p_index, "th element not exist! only ", p_list->length(), " element(s).");
            }
        }
        else if (auto * p_sym = boost::get<varlisp::symbol>(&*stem_it)) {
            p_env = const_cast<varlisp::Environment*>(boost::get<varlisp::Environment>(p_obj));
            if (!p_env) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "need an Environment here , but ", *stem_it);
            }
            p_obj = p_env->find(p_sym->name());
            if (!p_obj) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "field ", p_sym->name(), " not exist! use (symbol ...) to check");
            }
        }
        else {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": type error; ", p_obj->which(), " )");
        }
    }

    return *p_obj;
}

REGIST_BUILTIN("symbols", 0, 1, eval_symbols,
               "; symbols 返回当前，或者目标{}内部的所有标识符symbol列表\n"
               "(symbols) -> [current-symbol-list]]\n"
               "(symbols {}-name) -> [target-{}-symbol-list]");

Object eval_symbols(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "symbols";
    auto * p_env = &env;
    if (!args.empty()) {
        Object tmp;
        const varlisp::symbol* p_sym = varlisp::getSymbol(env, detail::car(args), tmp);
        if (!p_sym) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": need a symbol at first argument, but ", detail::car(args), ")");
        }
        auto location = detail::json_accessor::locate(env, *p_sym);
        if (!location.first) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": symbol ", *p_sym, " cannot be found)");
        }
        p_env = boost::get<varlisp::Environment>(location.first);
        if (!p_env) {
            SSS_POSITION_THROW(std::runtime_error,
                               "(", funcName, ": symbol ", *p_sym, " is not to a context)");
        }
    }
    auto symbols = varlisp::List::makeSQuoteList();
    auto back_it = detail::list_back_inserter<Object>(symbols);
    for (auto it = p_env->begin(); it != p_env->end(); ++it) {
        *back_it++ = std::move(varlisp::symbol(it->first));
    }
    return symbols;
}

} // namespace varlisp
