#include <cmath>
#include <sstream>

#include <sss/util/PostionThrow.hpp>
#include <sss/log.hpp>
#include <sss/iConvpp.hpp>
#include <sss/encoding.hpp>
#include <sss/path.hpp>

#include <ss1x/asio/GetFile.hpp>
#include <ss1x/asio/headers.hpp>
#include <ss1x/asio/utility.hpp>

#include "builtin.hpp"
#include "cast2double_visitor.hpp"
#include "eval_visitor.hpp"
#include "print_visitor.hpp"
#include "environment.hpp"
#include "strict_equal_visitor.hpp"
#include "strict_less_visitor.hpp"

namespace varlisp {

    enum type_t {
        TYPE_ADD,
        TYPE_SUB,
        TYPE_MUL,
        TYPE_DIV,

        TYPE_POW,

        TYPE_EQ,

        TYPE_GT,
        TYPE_LT,

        TYPE_GE,
        TYPE_LE,

        TYPE_EVAL,

        TYPE_READ,
        TYPE_WRITE,

        TYPE_SPLIT,
        TYPE_ZIP,

        TYPE_HTTP_GET,
        TYPE_GUMBO_QUERY
    };

    void ensure_utf8(std::string& content, const std::string& encodings)
    {
        std::string encoding = sss::Encoding::encodings(content, encodings);
        if (encoding.empty()) {
            encoding = sss::Encoding::dectect(content);
        }

        if (!sss::Encoding::isCompatibleWith(encoding, "utf8")) {
            std::string out;
            sss::iConv ic("utf8", encoding);
            ic.convert(out, content);
            std::swap(out, content);
        }
    }

    typedef Object (*eval_func_t)(varlisp::Environment& env, const varlisp::List& args);

    Object eval_add(varlisp::Environment& env, const varlisp::List& args)
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        double sum = 0;
        const List * p = &args;
        while (p && p->head.which()) {
            sum += boost::apply_visitor(cast2double_visitor(env), p->head);
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, sum);
        return Object(sum);
    }

    Object eval_sub(varlisp::Environment& env, const varlisp::List& args)
    {
        int args_cnt = args.length();
        if (args_cnt == 1) {
            return Object(-boost::apply_visitor(cast2double_visitor(env),
                                                args.head));
        }
        else {
            double sum = boost::apply_visitor(cast2double_visitor(env), args.head);
            const List * p = &args.tail[0];
            while (p && p->head.which()) {
                sum -= boost::apply_visitor(cast2double_visitor(env), p->head);
                if (p->tail.empty()) {
                    p = 0;
                }
                else {
                    p = &p->tail[0];
                }
            }
            return Object(sum);
        }
    }

    Object eval_mul(varlisp::Environment& env, const varlisp::List& args)
    {
        double mul = boost::apply_visitor(cast2double_visitor(env), args.head);
        if (args.head.which() == 5) {
            const Object& ref = env[boost::get<varlisp::symbol>(args.head).m_data];
        }
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, mul);
        const List * p = &args.tail[0];
        while (mul && p && p->head.which()) {
            mul *= boost::apply_visitor(cast2double_visitor(env), p->head);
            SSS_LOG_EXPRESSION(sss::log::log_DEBUG, mul);
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
        return Object(mul);
    }

    Object eval_div(varlisp::Environment& env, const varlisp::List& args)
    {
        double mul = boost::apply_visitor(cast2double_visitor(env), args.head);
        const List * p = &args.tail[0];
        while (mul && p && p->head.which()) {
            double div = boost::apply_visitor(cast2double_visitor(env), p->head);
            if (!div) {
                throw std::runtime_error(" divide by zero!");
            }
            mul /= div;
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
        return Object(mul);
    }

    Object eval_pow(varlisp::Environment& env, const varlisp::List& args)
    {
        double lhs = boost::apply_visitor(cast2double_visitor(env), args.head);
        double rhs = boost::apply_visitor(cast2double_visitor(env), args.tail[0].head);

        return Object(std::pow(lhs, rhs));
    }

    // 对于drracket来说，比较运算符，它会直接要求转换到real域；
    // 如果失败，则抛出异常；
    // 即，lambda函式，与1.2实数的比较大小，会出错，抛出异常：
    // > (if (> fib 1) 1 2)
    // >: contract violation
    //   expected: real?
    //   given: #<procedure:fib>
    //   argument position: 1st
    //   other arguments...:
    //    1
    // 其中：
    // > (define fib (lambda (x) (if (> x 2) (+ (fib (- x 1)) (fib (- x 2))) 1)))
    Object eval_eq(varlisp::Environment& env, const varlisp::List& args)
    {
        return Object(boost::apply_visitor(strict_equal_visitor(env),
                                           args.head,
                                           args.tail[0].head));
    }

    Object eval_gt(varlisp::Environment& env, const varlisp::List& args)
    {
        return Object(
            !boost::apply_visitor(strict_equal_visitor(env),
                                  args.tail[0].head,
                                  args.head) &&
            boost::apply_visitor(strict_less_visitor(env),
                                 args.tail[0].head,
                                 args.head));
    }

    Object eval_lt(varlisp::Environment& env, const varlisp::List& args)
    {
        return Object(boost::apply_visitor(strict_less_visitor(env),
                                           args.head,
                                           args.tail[0].head));
    }

    Object eval_ge(varlisp::Environment& env, const varlisp::List& args)
    {
        return Object(boost::apply_visitor(strict_less_visitor(env),
                                           args.tail[0].head,
                                           args.head));
    }

    Object eval_le(varlisp::Environment& env, const varlisp::List& args)
    {
        return Object(
            boost::apply_visitor(strict_equal_visitor(env),
                                 args.tail[0].head,
                                 args.head)
            ||
            boost::apply_visitor(strict_less_visitor(env),
                                 args.head,
                                 args.tail[0].head));

    }

    Object eval_eval(varlisp::Environment& env, const varlisp::List& args)
    {
        int arg_length = args.length();
        if (arg_length == 2) {
            SSS_POSTION_THROW(std::runtime_error,
                              "eval only support one arguments now!");
            // TODO
            // 至于环境变量，可以根据传入进来的symbol，然后通过全局函数，获取环
            // 境变量的引用，然后应用到这里。
        }
        // 需要先对参数eval一次，然后再eval一次！
        // 为什么需要两次？
        // 因为，既然是通过eval调用进来的，那么，这个eval，根据lisp的语法，必须
        // 先解析()内部，然后作为外部的参数再次解析。
        //
        // 所以，这两次eval动作，分别是原有的用户动作，和这个eval关键字自己引发的动作；
        varlisp::Object first_res = boost::apply_visitor(eval_visitor(env), args.head);
        return boost::apply_visitor(eval_visitor(env), first_res);
    }

    Object eval_read(varlisp::Environment& env, const varlisp::List& args)
    {
        Object path = boost::apply_visitor(eval_visitor(env), args.head);
        const std::string *p_path = boost::get<std::string>(&path);
        if (!p_path) {
            SSS_POSTION_THROW(std::runtime_error,
                              "read requies a path");
        }
        std::string full_path = sss::path::full_of_copy(*p_path);
        if (sss::path::file_exists(full_path) != sss::PATH_TO_FILE) {
            SSS_POSTION_THROW(std::runtime_error,
                              "path `" << *p_path << "` not to file");
        }
        // varlisp::List content;
        // std::string line;
        std::string content;
        sss::path::file2string(full_path, content);

        // 对于小文件，其判断可能出错
        // ensure_utf8(content, "cp936,utf8");

        return content;
    }

    Object eval_write(varlisp::Environment& env, const varlisp::List& args)
    {
        Object content = boost::apply_visitor(eval_visitor(env), args.head);

        const std::string *p_content = boost::get<std::string>(&content);
        if (!p_content) {
            SSS_POSTION_THROW(std::runtime_error,
                              "write requies content to parsing");
        }
        Object path = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_path = boost::get<std::string>(&path);
        if (!p_path) {
            SSS_POSTION_THROW(std::runtime_error,
                              "write requies path to write");
        }
        std::string full_path = sss::path::full_of_copy(*p_path);
        sss::path::mkpath(sss::path::dirname(full_path));
        std::ofstream ofs(full_path, std::ios_base::out | std::ios_base::binary);
        if (!ofs.good()) {
            SSS_POSTION_THROW(std::runtime_error,
                              "write failed open file to write");
        }
        ofs << *p_content;
        return Object();
    }

    Object eval_split(varlisp::Environment& env, const varlisp::List& args)
    {
    }

    Object eval_zip(varlisp::Environment& env, const varlisp::List& args)
    {
    }

    Object eval_http_get(varlisp::Environment& env, const varlisp::List& args)
    {
        Object url = boost::apply_visitor(eval_visitor(env), args.head);
        const std::string *p_url = boost::get<std::string>(&url);
        if (!p_url) {
            SSS_POSTION_THROW(std::runtime_error,
                              "http-get requie url for downloading!");
        }

        std::ostringstream oss;
        ss1x::http::Headers headers;
        ss1x::asio::getFile(oss, headers, *p_url);

        std::string content = oss.str();
        std::string charset = headers.get("Content-Type", "charset");
        if (charset.empty()) {
            charset = "cp936,utf8";
        }

        ensure_utf8(content, charset);

        return content;
    }

    Object eval_gumbo_query(varlisp::Environment& env, const varlisp::List& args)
    {
        Object content = boost::apply_visitor(eval_visitor(env), args.head);
        const std::string *p_content = boost::get<std::string>(&content);
        if (!p_content) {
            SSS_POSTION_THROW(std::runtime_error,
                              "gumbo-query requies content to parsing");
        }
        Object query = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_query = boost::get<std::string>(&query);
        if (!p_query) {
            SSS_POSTION_THROW(std::runtime_error,
                              "gumbo-query requies query string");
        }
        std::ostringstream oss;
        ss1x::util::html::queryText(oss,
                                    *p_content,
                                    *p_query);

        return oss.str();
    }

    // 参数格式；
    // 闭区间；-1表示无穷
    struct builtin_info_t {
        const char *    name;
        int             min;
        int             max;
        eval_func_t     eval_fun;
    };

    const builtin_info_t builtin_infos[]
        = {
            {"+",       1, -1, &eval_add},
            {"-",       1, -1, &eval_sub},
            {"*",       2, -1, &eval_mul},
            {"/",       2, -1, &eval_div},
            {"^",       2,  2, &eval_pow},
            {"=",       2,  2, &eval_eq},
            {">",       2,  2, &eval_gt},
            {"<",       2,  2, &eval_lt},
            {">=",      2,  2, &eval_ge},
            {"<=",      2,  2, &eval_le},
            {"eval",    1,  2, &eval_eval},

            {"read",    1,  1, &eval_read},
            {"write",   2,  2, &eval_write},

            {"split",   1,  1, &eval_split},
            {"zip",     2,  2, &eval_zip},

            {"http-get",        1,  1, &eval_http_get},
            {"gumbo-query",     2,  2, &eval_gumbo_query},
        };

    void Builtin::print(std::ostream& o) const
    {
        o << "#<builtin:\"" << builtin_infos[this->m_type].name << "\">";
    }

    Builtin::Builtin(int type)
        : m_type(type)
    {
    }

    Object Builtin::eval(varlisp::Environment& env, const varlisp::List& args) const
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, args);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, builtin_infos[m_type].name);
        int arg_length = args.length();
        int arg_min = builtin_infos[m_type].min;
        int arg_max = builtin_infos[m_type].max;
        if (arg_min > 0 && arg_length < arg_min) {
            SSS_POSTION_THROW(std::runtime_error,
                              " need at least " << arg_min << " parameters. "
                              << " but provided " << arg_length);
        }
        if (arg_max > 0 && arg_length > arg_max) {
            SSS_POSTION_THROW(std::runtime_error,
                              " need at most " << arg_max << " parameters."
                              << " but provided " << arg_length);
        }
        return builtin_infos[m_type].eval_fun(env, args);
    }

    void Builtin::regist_builtin_function(Environment& env)
    {
        env["+"]    = varlisp::Builtin(varlisp::TYPE_ADD);
        env["-"]    = varlisp::Builtin(varlisp::TYPE_SUB);
        env["*"]    = varlisp::Builtin(varlisp::TYPE_MUL);
        env["/"]    = varlisp::Builtin(varlisp::TYPE_DIV);

        env["^"]    = varlisp::Builtin(varlisp::TYPE_POW);

        env["="]    = varlisp::Builtin(varlisp::TYPE_EQ);

        env[">"]    = varlisp::Builtin(varlisp::TYPE_GT);
        env["<"]    = varlisp::Builtin(varlisp::TYPE_LT);
        env[">="]   = varlisp::Builtin(varlisp::TYPE_GE);
        env["<="]   = varlisp::Builtin(varlisp::TYPE_LE);

        env["eval"] = varlisp::Builtin(varlisp::TYPE_EVAL);

        env["read"] = varlisp::Builtin(varlisp::TYPE_READ);
        env["write"] = varlisp::Builtin(varlisp::TYPE_WRITE);

        env["split"] = varlisp::Builtin(varlisp::TYPE_SPLIT);
        env["zip"] = varlisp::Builtin(varlisp::TYPE_ZIP);

        env["http-get"]     = varlisp::Builtin(varlisp::TYPE_HTTP_GET);
        env["gumbo-query"]  = varlisp::Builtin(varlisp::TYPE_GUMBO_QUERY);
    }
} // namespace varlisp
