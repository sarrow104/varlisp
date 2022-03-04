#include <sss/debug/value_msg.hpp>

#include "../object.hpp"

#include "../builtin_helper.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("catch", 2, -1, eval_catch,
               "; catch 异常捕获\n"
               "; 接受2-n个参数。\n"
               "; 参数1 表示可能发生异常的，待执行的语句；\n"
               "; 参数2-n (type error_handle) 表示捕获对；\n"
               ";       type\n"
               ";         必须是基本类型——因为异常的处理，不再涉及\"环境\"；\n"
               ";       error_handle\n"
               ";         可选参数，表示捕获到的异常值的处理过程。\n"
               ";       如果在执行语句expr的时候没有抛出异常，那么(catch ...)\n"
               ";       返回expr的值。\n"
               ";       如果抛出异常，并且与type相同类型，那么当做捕获成功。\n"
               ";       此时，如果提供了参数3，那么返回\n"
               ";       (apply error_handle [exception_value]) 的结果。\n"
               ";       如果没有提供error_handle，则直接返回异常值。\n"
               ";\n"
               ";       如果expr抛出的类型不正确，则不会执行相应的 \n"
               ";       error_handle 语句，而是继续向上抛出异常。一直到捕\n"
               ";       获成功，或者等到行交互工具，完成最后的捕获。\n"
               "; 特例：正则表达式的捕获类型，只会捕获字符串！并且，会用正则\n"
               ";       表达式去匹配字符串；如果匹配失败，同样会继续抛出；也\n"
               ";       就是说，只有正则表达式的捕获类型的写法，才会考虑捕获\n"
               ";       表达式的值，其他情况，只会考虑类型。\n"
               "(catch expr (value1)...)\n"
               "        -> result-of-expr | exception-value\n"
               "(catch expr (value1 on_except_type1)...)\n"
               "        -> result-of-expr | result-of-on_value_handle-on-exception-value");
// (catch
//  expr
//  (typ1 on_error1)
//  (typ2 on_error2)
//  ...
// )

Object eval_catch(varlisp::Environment& env, const varlisp::List& args)
{
    try {
        std::array<Object, 1> objs;
        return varlisp::getAtomicValue(env, args.nth(0), objs[0]);
    }
    catch (Object& exception) {
        for (size_t i = 1; i != args.length(); ++i) {

            auto *p_vc_pair = boost::get<varlisp::List>(&args.nth(i));
            varlisp::requireOnFaild<varlisp::QuoteList>(p_vc_pair, "catch", 0, DEBUG_INFO);
            std::array<Object, 2> objs;
            const Object& require_type = getAtomicValue(env, p_vc_pair->nth(0), objs[0]);
            if (exception.which() == require_type.which()) {
                if (p_vc_pair->length() >= 2) {
                    // FIXME 处理异常的过程，不应该再抛出异常！任何！
                    const auto& func = varlisp::getAtomicValue(env, p_vc_pair->nth(1), objs[1]);
                    varlisp::List raw_arg;
                    raw_arg.append(exception);
                    return varlisp::apply(env, func, raw_arg);
                }
                else {
                    return exception;
                }
            }
            else if (auto * p_regex = boost::get<varlisp::regex_t>(&require_type)) {
                auto * p_string = boost::get<varlisp::string_t>(&exception);
                if (p_string && RE2::PartialMatch(*p_string, *(*p_regex))) {
                    return exception;
                }
            }
        }
        COLOG_DEBUG("catch all passed");
        throw;
    }
    catch (...) {
        COLOG_DEBUG("catch failed");
        throw;
    }
}

REGIST_BUILTIN("throw", 1, 1, eval_throw,
               "; throw 抛出异常值\n"
               "(throw value) -> nil");

Object eval_throw(varlisp::Environment& env, const varlisp::List& args)
{
    std::array<Object, 1> objs;
    const Object& result = varlisp::getAtomicValue(env, args.nth(0), objs[0]);
    COLOG_ERROR(result);
    throw result;
}

REGIST_BUILTIN("std-exception", 1, 1, eval_std_exception,
               "; std-exception 将std::xxxx标准异常，转换list\n"
               "; 该list的第一个元素，是异常的字符串形式，第二个元素，是后续的msg\n"
               "(std-exception expr) -> result-of-expr");

Object eval_std_exception(varlisp::Environment& env, const varlisp::List& args)
{
    try {
        std::array<Object, 1> objs;
        return varlisp::getAtomicValue(env, args.nth(0), objs[0]);
    }
    catch (std::runtime_error& e)
    {
        throw Object(varlisp::List::makeSQuoteList(varlisp::string_t("std:runtime_error"),
                                                   varlisp::string_t(e.what())));
    }
    catch (std::exception& e)
    {
        throw Object(varlisp::List::makeSQuoteList(varlisp::string_t("std:exception"),
                                                   varlisp::string_t(e.what())));
    }
    catch (...)
    {
        throw;
    }
}

} // namespace varlisp
