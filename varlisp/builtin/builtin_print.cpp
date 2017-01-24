#include <array>

#include <sss/raw_print.hpp>
#include <sss/util/PostionThrow.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/colorlog.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../fmtArgInfo.hpp"
#include "../fmt_print_visitor.hpp"
#include "../detail/io.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"

namespace varlisp {

void fmt_impl(std::ostream& oss, varlisp::Environment& env,
              const varlisp::List& args, const char* funcName)
{
    std::array<Object, 1> objs;
    const string_t* p_fmt =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::vector<fmtArgInfo> fmts;
    std::vector<sss::string_view> padding;
    parseFmt(p_fmt, fmts, padding);

    size_t arg_len = args.length() - 1;
    std::vector<const Object*> vecObjPtr;
    std::vector<const Object*> vecArgPtr;
    std::vector<Object> vecTmpArgs;
    vecTmpArgs.resize(arg_len, Object{});
    vecObjPtr.resize(arg_len, nullptr);

    const varlisp::List arg = args.tail();
    for (auto it = arg.begin(); it != arg.end(); ++it) {
        vecArgPtr.push_back(&(*it));
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

REGIST_BUILTIN("io-print", 1, -1, eval_print, "(io-print \"fmt\" ...)");

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

REGIST_BUILTIN("io-print-ln", 1, -1, eval_print_ln, "(io-print \"fmt\n\" ...)");

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

REGIST_BUILTIN("io-fmt", 1, -1, eval_fmt,
               "(fmt \"fmt-str\" arg1 arg2 ... argn) -> \"fmt-out\"");

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

REGIST_BUILTIN("format", 1, -1, eval_format,
               "; format 按照python风格格式化一个fmt格式串；\n"
               "; 如果out-fd是nil的话，则返回一个格式化后的字符串。\n"
               "; 如果out-fd合法的话，则输出到对应的fd上；比如1,2分别\n"
               "; 是stdout,stderr;\n"
               "; 如果不合法，……\n"
               "; 格式说明\n"
               "; format_spec ::= [[fill]align][sign][#][0][width][,][.precision][type]\n"
               "; fill        ::= <any character>\n"
               "; align       ::= '<' | '>' | '=' | '^'\n"
               ";   '=' ->  ‘+000000120’\n"
               "; sign        ::=  '+' | '-' | ' '\n"
               ";   '+' 始终添加正负号；\n"
               ";   '-' 仅负数添加负号；\n"
               ";   ' ' 正数的正号，用空格代替——以保证正负数，尽量对齐……\n"
               "; width       ::=  integer\n"
               "; precision   ::=  integer\n"
               ";   仅对浮点数有用；\n"
               "; type        ::=  'b' | 'c' | 'd' | 'e' | 'E' | 'f' | 'F' | 'g' | 'G' | 'n' |\n"
               ";                  'o' | 's' | 'x' | 'X' | '%'\n"
               ";  'b' 二进制；\n"
               ";  'c' 将数字，在打印前，转换成对应的unicode字符；\n"
               ";  'd' 十进制数字；\n"
               ";  'o' 八进制数字；\n"
               ";  'x' 十六进制；小写；\n"
               ";  'X' 十六进制，大写\n"
               ";  'n' 同'd'；但是用local来打印，比如按1000进位，用逗号分割；\n"
               ";  'e' 浮点数指数，小写；\n"
               ";  'E' 浮点数指数，大写；\n"
               ";  'f' 固定小数点；默认精度6；\n"
               ";  'F' 同'f'\n"
               ";  'g' 科学计数法\n"
               ";  'G' 科学计数法；大写'E'；\n"
               ";  '%' 百分号形式，打印浮点数；附带'%'\n"
               "(format out-fd \"fmt\" ...) -> ...");

/**
 * @brief
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_format(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "format";
    Object objFd;
    const Object& fdRef = varlisp::getAtomicValue(env, detail::car(args), objFd);
    int64_t fd = -1;
    if (nullptr != boost::get<varlisp::Nill>(&fdRef)) {
        fd = 0;
    }
    else if (const int64_t* p_fd = boost::get<int64_t>(&fdRef)) {
        if (*p_fd <= 0) {
            SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                               ": requies positive int64_t as 1st argument)");
        }
        fd = *p_fd;
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, "(", funcName,
                           ": requies int64_t fd or nil as 1st argument)");
    }

    std::ostringstream oss;
    fmt_impl(oss, env, args.tail(), funcName);
    std::string out = oss.str();

    if (fd == 0) {
        return string_t{std::move(out)};
    }
    else {
        varlisp::detail::writestring(fd, sss::string_view{out});
        return varlisp::Nill{};
    }
}

REGIST_BUILTIN("fmt-escape", 1, 1, eval_fmt_escape,
               "; fmt-escape 转义可能被python风格误解的文本串并返回\n"
               "(fmt-escape \"normal-string-may-have-curly-bracket\") ->\n"
               " \"scaped-string\"");

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
    std::array<Object, 1> objs;
    const string_t* p_fmt =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
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
