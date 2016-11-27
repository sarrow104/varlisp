#include "object.hpp"

#include <sss/raw_print.hpp>
#include <sss/util/PostionThrow.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/colorlog.hpp>

#include "builtin_helper.hpp"
#include "fmtArgInfo.hpp"
#include "fmt_print_visitor.hpp"

namespace varlisp {

void fmt_impl(std::ostream& oss, varlisp::Environment& env, const varlisp::List& args, const char * funcName)
{
    Object obj1;
    const string_t* p_fmt = getTypedValue<string_t>(env, args.head, obj1);
    if (!p_fmt) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires string to escape at 1st)");
    }
    std::vector<fmtArgInfo> fmts;
    std::vector<sss::string_view> padding;
    parseFmt(p_fmt, fmts, padding);

    size_t arg_len = args.length() - 1;
    std::vector<const Object*> vecObjPtr;
    std::vector<const Object*> vecArgPtr;
    std::vector<Object> vecTmpArgs;
    vecTmpArgs.resize(arg_len, Object{});
    vecObjPtr.resize(arg_len, nullptr);

    const varlisp::List* p_arg = args.next();
    while (p_arg) {
        if (!p_arg->head.which()) {
            SSS_POSITION_THROW(std::runtime_error, "varlisp::Empty");
        }
        vecArgPtr.push_back(&p_arg->head);
        p_arg = p_arg->next();
    }

    COLOG_DEBUG(SSS_VALUE_MSG(arg_len));
    size_t i = 0;
    for (; i < padding.size(); ++i) {
        oss << padding[i];
        if (i >= fmts.size()) {
            continue;
        }
        size_t arg_ref_id = fmts[i].index;
        COLOG_DEBUG(SSS_VALUE_MSG(arg_ref_id));
        if (arg_ref_id == sss::string_view::npos) {
            continue;
        }
        if (arg_ref_id >= arg_len) {
            SSS_POSITION_THROW(std::runtime_error, "ref arg-id", arg_ref_id,
                              " out of range [0,", arg_len, ')');
        }
        if (!vecObjPtr[arg_ref_id]) {
            vecObjPtr[arg_ref_id] = &getAtomicValue(env, *vecArgPtr[arg_ref_id],
                                                    vecTmpArgs[arg_ref_id]);
        }
        boost::apply_visitor(fmt_print_visitor(oss, fmts[i]),
                             *vecObjPtr[arg_ref_id]);
    }
}

/**
 * @brief (io-print "fmt" ...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_print(varlisp::Environment& env, const varlisp::List& args)
{
    fmt_impl(std::cout, env, args, "io-print");
    return Object{Empty{}};
}

/**
 * @brief (io-print "fmt\n" ...)
 *
 * @param[in] env
 * @param[in] args
 *
 * @return 
 */
Object eval_print_ln(varlisp::Environment& env, const varlisp::List& args)
{
    fmt_impl(std::cout, env, args, "io-print-ln");
    std::cout << std::endl;
    return Object{Empty{}};
}

/**
 * @brief (fmt "fmt-str" arg1 arg2 ... argn) -> "fmt-out"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_fmt(varlisp::Environment& env, const varlisp::List& args)
{
    std::ostringstream oss;
    fmt_impl(oss, env, args, "io:fmt");
    std::string out = oss.str();
    return Object{string_t{std::move(out)}};
}

/**
 * @brief (fmt-escape "normal-string-may-have-curly-bracket") -> "scaped-string"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_fmt_escape(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "fmt-escape";
    Object obj1;
    const string_t* p_fmt = getTypedValue<string_t>(env, args.head, obj1);
    if (!p_fmt) {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                          ": requires string to escape at 1st)");
    }
    std::string escaped_str;
    escaped_str.reserve(p_fmt->size());
    for (auto i : *p_fmt) {
        switch (i) {
            case '{':
                escaped_str += "{{";
                break;
            case '}':
                escaped_str += "}}";
                break;

            default:
                escaped_str += i;
                break;
        }
    }
    return Object{string_t{std::move(escaped_str)}};
}

// NOTE ruby中，有这种格式串：
//
// "this is fromat #{var_name}"，会在构造字符串的时候，将环境中变量
// var_name的值，写入对应位置，形成一个新的字符串。
// 在varLisp中，也可以达到这种效果。

}  // namespace varlisp
