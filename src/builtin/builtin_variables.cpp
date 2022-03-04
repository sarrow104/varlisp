#include <set>
#include <array>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/array_view.hpp>
#include <vector>

// #include <gsl/array_view.hpp>

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

// 如果要删除的，位于末尾或者头部，则直接调整start/end 即可；
// 否则，查看引用计数；如果仅自己，则原地修改；
// 否则，新建一个，再拷贝；
void eraseSListItem(varlisp::List& sList, int index) {
    auto uqList = sList.unquoteType<varlisp::List>();
    if (uqList == nullptr) {
        SSS_POSITION_THROW(std::runtime_error, "uqList == nullptr");
    }
    if (uqList->size() == 0) {
        SSS_POSITION_THROW(std::runtime_error, "uqList->size() == 0");
    }

    if (index < 0) {
        if (index < - uqList->size()) {
            SSS_POSITION_THROW(std::runtime_error, "uqList->size() == 0");
        }
        index += uqList->size();
    }

    if (uqList->size() <= index) {
        SSS_POSITION_THROW(std::runtime_error, "uqList->size() <= index");
    }

    if (index == 0) {
        uqList->pop_front();
        return;
    } else if (index == uqList->size() - 1) {
        uqList->pop_back();
        return;
    }

    varlisp::List newList{};

    for (int i = 0; i != uqList->size(); ++i) {
        if (i == index) { continue; }
        newList.append(uqList->nth(i));
    }
    varlisp::Object o{newList};

    sList.nth(1).swap(o);
}

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
    try {
        auto location = jc.locate(env);
        if (!location.obj) {
            COLOG_INFO("tobe delete symbol ", p_sym->name(), " not exist");
            return ret;
        }
        if (jc.has_sub()) {
            // for named field
            // - : {albeta}+
            if (!detail::is_index(jc.stems().back())) {
                location.env->erase(jc.stems().back());
                ret = true;
            }
            // for index
            //  - : {digit}+     | from 0; means the first one
            //  - : '-' {digit}+ | from -1; means the last one
            else {
                auto lastIndex  = sss::string_cast<int>(jc.stems().back());
                // > (define l [[1 2] [3 4] [5 6]])
                // l
                // > (undef l:2:1)
                // [E] /U/s/u/p/L/v/s/s/b/builtin_variables.cpp:77 eval_undef(): [2, 1]
                // [E] /U/s/u/p/L/v/s/s/b/builtin_variables.cpp:78 eval_undef(): 6
                // #f
                eraseSListItem(*location.list, lastIndex);
                ret = true;
            }
        }
        else {
            location.env->erase(p_sym->name());
            ret = true;
        }
        return ret;
    }
    catch(std::runtime_error& e) {
        COLOG_ERROR(e.what());
        return false;
    }
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
    Object tmp;
    return bool(varlisp::findSymbolDeep(env, args.nth(0), tmp, funcName));
}

REGIST_BUILTIN("var-list", 0, 1, eval_var_list,
               "打印当前环境，以及所有父环境下所有变量；并返回对象计数\n"
               "(var-list) -> int64_t\n"
               "打印提供的环境名所拥有的变量；并返回对象计数\n"
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
               "   ((x 2)\n"
               "    (y (pow x 3))\n"
               "    (z (pow x 4)))\n"
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
               "(setq symbol1 expr1 symbol2 expr2 ... ) -> value-of-last-expr");

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
        // FIXME
        auto & x = varlisp::getAtomicValue(env, args.nth(i + 1), res);
        COLOG_DEBUG(*p_value, x);
        *p_value = std::move(x);
    }
    if (!p_value) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": last value nullptr error!)");
    }
    return *p_value;
}

REGIST_BUILTIN("setf", 2, 2, eval_setf,
               "; setf 在clisp等，里面是使用了setq的宏；这里是在当前环境，定义或者修改一个变量\n"
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

    const varlisp::symbol* p_sym =
        boost::get<varlisp::symbol>(&args.nth(0));
    varlisp::requireOnFaild<varlisp::symbol>(p_sym, funcName, 0, DEBUG_INFO);

    Object value;
    const Object& result = varlisp::getAtomicValue(env,
                                                   args.nth(1),
                                                   value);
    env[p_sym->name()] = result;

    return result;
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

REGIST_BUILTIN("locate", 2, 3, eval_locate,
               "; locate 在某{}或者[]，按照json-accessor的语法，返回子对象的值\n"
               "; - 允许字面值\n"
               "; - 对于不存在的下标索引或者字段名，将抛出异常\n"
               "; - 对于不存在的下标索引或者字段名，传入默认值的版本，将返回默认值\n"
               "(locate {}-or-[]-value '(string-or-int...)) -> sub-value\n"
               "(locate {}-or-[]-value '(string-or-int...) default-value) -> sub-value-or-default-value");

// TODO 如何用类似的语法，进行更改值呢？
Object eval_locate(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "locate";
    Object tmp;
    Object tmpDefault;
    const Object& objRef = varlisp::getAtomicValue(env, detail::car(args), tmp);

    Object tmpStems;
    auto p_stem_list = varlisp::getQuotedList(env, detail::cadr(args), tmpStems);
    if (!p_stem_list) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", funcName, ": 2nd argument must be a s-list; but ", detail::cadr(args), " )");
    }

    const Object* p_default = nullptr;
    if (args.length() == 3) {
        p_default = &varlisp::getAtomicValue(env, args.nth(2), tmpDefault);
    }

    auto * p_obj = &objRef;
    auto * p_env = &env;

    for (auto stem_it = p_stem_list->begin(); stem_it != p_stem_list->end(); ++stem_it) {
        Object tmp2;
        const auto & locator = getAtomicValue(env, *stem_it, tmp2);
        if (auto * p_index = boost::get<int64_t>(&locator)) {
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
                if (p_default) {
                    p_obj = p_default;
                    break;
                }
                SSS_POSITION_THROW(std::runtime_error, *p_index,
                                   "th element not exist! only ",
                                   p_list->length(), " element(s).");
            }
        }
        else if (auto * p_str = boost::get<string_t>(&locator)) {
            p_env = const_cast<varlisp::Environment*>(boost::get<varlisp::Environment>(p_obj));
            if (!p_env) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "need an Environment here , but ", *stem_it);
            }
            std::string name = *p_str->gen_shared();
            p_obj = p_env->find(name);
            if (!p_obj) {
                if (p_default) {
                    p_obj = p_default;
                    break;
                }
                SSS_POSITION_THROW(std::runtime_error, "field ", name,
                                   " not exist! use (symbol ...) to check");
            }
        }
        else if (auto * p_slist = boost::get<varlisp::List>(&locator)) {
            if (!p_slist->is_quoted()) {
                SSS_POSITION_THROW(std::runtime_error, "slist required!");
            }
            auto * p_sym = p_slist->unquoteType<varlisp::symbol>();
            p_env = const_cast<varlisp::Environment*>(boost::get<varlisp::Environment>(p_obj));
            if (!p_env) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "need an Environment here , but ", *stem_it);
            }
            p_obj = p_env->find(p_sym->name());
            if (!p_obj) {
                if (p_default) {
                    p_obj = p_default;
                    break;
                }
                SSS_POSITION_THROW(std::runtime_error, "field ", p_sym->name(),
                                   " not exist! use (symbol ...) to check");
            }
        }
        else {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": type error; ", stem_it->which(), " at ",
                               stem_it - p_stem_list->begin(), " )");
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
