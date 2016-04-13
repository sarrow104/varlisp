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

#include <sss/spliter.hpp>

#include "builtin.hpp"
#include "cast2double_visitor.hpp"
#include "eval_visitor.hpp"
#include "print_visitor.hpp"
#include "environment.hpp"
#include "strict_equal_visitor.hpp"
#include "strict_less_visitor.hpp"
#include "parser.hpp"
#include "interpreter.hpp"

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
        TYPE_LOAD,

        TYPE_READ,
        TYPE_WRITE,
        TYPE_WRITE_APPEND,

        TYPE_SPLIT,
        TYPE_JOIN,

        TYPE_HTTP_GET,
        TYPE_GUMBO_QUERY,

        TYPE_REGEX,
        TYPE_REGEX_MATCH,
        TYPE_REGEX_SEARCH,
        TYPE_REGEX_REPLACE,

        TYPE_REGEX_SPLIT,
        TYPE_REGEX_COLLECT,

        TYPE_SUBSTR,
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

    // TODO
    Object eval_load(varlisp::Environment& env, const varlisp::List& args)
    {
        // NOTE 既然 load是内建函数，那么load，就可以出现在任何地方；比如另外一个脚本；
        // 而，我的解释器，对于脚本的load，本质上，是让内部的Tokenizer对象，
        // push、pop一个额外的工作栈——但是，当前，只是，Tokenizer只拥有两个工作栈，相当于双缓冲区；
        // 对于嵌套load的使用，这可能会力不从心！
        //
        // 于是，安全的作法有两种：
        //      1. 让Tokenizer对象，成为真正的多栈的对象；
        //      2. 按需要，临时构建Tokenizer对象，以供解析用——相当于有了Tokenizer的堆栈；
        //
        // NOTE 另外，在load的时候，是否需要新建一个Environment对象呢？
        // 这是作用域的问题；即，对于(load "path/to/script")语句而言，新解析到
        // 的标识符(对象、函数等等)，应该放到哪个Environment中呢？

#if 1
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

        varlisp::Interpreter * p_inter = env.getInterpreter();
        if (!p_inter) {
            SSS_POSTION_THROW(std::runtime_error, "env.getInterpreter return 0 ptr");
        }
        varlisp::Parser & parser = p_inter->get_parser();
        parser.parse(env, content, true);
#endif
        return Object();
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

    Object eval_write_impl(varlisp::Environment& env, const varlisp::List& args, bool append)
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
        auto bit_op = std::ios_base::out | std::ios_base::binary;
        if (append) {
            bit_op |= std::ios_base::app;
        }
        std::ofstream ofs(full_path, bit_op);
        if (!ofs.good()) {
            SSS_POSTION_THROW(std::runtime_error,
                              "write failed open file to write");
        }
        ofs << *p_content;
        return Object();
    }

    Object eval_write(varlisp::Environment& env, const varlisp::List& args)
    {
        return eval_write_impl(env, args, false);
    }

    Object eval_write_append(varlisp::Environment& env, const varlisp::List& args)
    {
        return eval_write_impl(env, args, true);
    }

    /**
     * @brief 拆分字符串
     *
     * @param [in]env
     * @param [in]args 支持两个，参数，分别待切分字符串，和分割字符串；
     *
     * @return 分割之后的列表；
     *
     * TODO 支持正则表达式，确定sep!
     * 需要三个参数；
     * 其中第三个参数是表示正则的的symbol
     */
    Object eval_split(varlisp::Environment& env, const varlisp::List& args)
    {
        Object content = boost::apply_visitor(eval_visitor(env), args.head);

        const std::string *p_content = boost::get<std::string>(&content);
        if (!p_content) {
            SSS_POSTION_THROW(std::runtime_error,
                              "split requies content to split");
        }
        std::string sep(1, ' ');
        if (args.length() == 2) {
            Object sep_obj = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
            const std::string *p_sep = boost::get<std::string>(&sep_obj);
            if (!p_sep) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "sep using for split must a string");
            }
            sep = *p_sep;
        }
        varlisp::List ret;
        std::string stem;
        if (sep.length() == 1) {
            sss::Spliter sp(*p_content, sep[0]);
            List * p_list = &ret;
            while (sp.fetch_next(stem)) {
                p_list = p_list->next_slot();
                p_list->head = stem;
            }
        }
        else {
            SSS_POSTION_THROW(std::runtime_error,
                              "split: sep.length() >= 2, not support yet!");
        }
        return Object(ret);
    }

    /**
     * @brief join string list
     *
     * @param [in] env
     * @param [in] args 第一个参数，必须是一个(list)；或者symbol
     *
     * @return 
     */
    Object eval_join(varlisp::Environment& env, const varlisp::List& args)
    {
        const List * p_list = 0;

        Object content = boost::apply_visitor(eval_visitor(env), args.head);

        p_list = boost::get<varlisp::List>(&content);

        if (!p_list) {
            SSS_POSTION_THROW(std::runtime_error,
                              "join: first must a list!");
        }

        std::string sep;
        if (args.length() == 2) {
            Object sep_obj = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
            const std::string *p_sep = boost::get<std::string>(&sep_obj);
            if (!p_sep) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "join: sep must be a string");
            }
            sep = *p_sep;
        }

        std::ostringstream oss;

        bool is_first = true;
        while (p_list && p_list->head.which()) {
            const std::string * p_stem = boost::get<std::string>(&p_list->head);
            if (!p_stem) {
                break;
            }
            if (is_first) {
                is_first = false;
            }
            else {
                oss << sep;
            }
            oss << *p_stem;
            p_list = p_list->next();
        }
        return Object(oss.str());
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

        if (args.length() == 3) {
            const std::string * p_proxy = boost::get<std::string>(&args.tail[0].head);
            if (!p_proxy) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "http-get 2nd parameter must be proxy domain string!");
            }
            const int * p_port = boost::get<int>(&args.tail[0].tail[0].head);
            if (!p_port) {
                SSS_POSTION_THROW(std::runtime_error,
                                  "http-get 3rd parameter must be proxy port number!");
            }
            ss1x::asio::proxyGetFile(oss, headers, *p_proxy, *p_port, *p_url);
        }
        else {
            ss1x::asio::getFile(oss, headers, *p_url);
        }

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

    Object eval_regex(varlisp::Environment& env, const varlisp::List& args)
    {
        Object content = boost::apply_visitor(eval_visitor(env), args.head);
        const std::string *p_content = boost::get<std::string>(&content);
        if (!p_content) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex: need one string to construct");
        }
        return sss::regex::CRegex(*p_content);
    }

    Object eval_regex_match(varlisp::Environment& env, const varlisp::List& args)
    {
        Object content = boost::apply_visitor(eval_visitor(env), args.head);
        sss::regex::CRegex *p_reg = boost::get<sss::regex::CRegex>(&content);
        if (!p_reg) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-match: regex obj");
        }
        Object target = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_content = boost::get<std::string>(&target);
        if (!p_content) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-match: need one string to match");
        }
        return p_reg->match(p_content->c_str());
    }

    /**
     * @brief (regex-search reg target offset = 0) -> (sub0, sub1...)
     *
     * @param env
     * @param args
     *
     * @return 
     */
    Object eval_regex_search(varlisp::Environment& env, const varlisp::List& args)
    {
        Object reg_obj = boost::apply_visitor(eval_visitor(env), args.head);
        sss::regex::CRegex *p_reg = boost::get<sss::regex::CRegex>(&reg_obj);
        if (!p_reg) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-search: regex obj");
        }

        Object target = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_content = boost::get<std::string>(&target);
        if (!p_content) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-search: need one target string to search");
        }

        int offset = 0;
        if (args.length() == 3) {
            Object offset_obj = boost::apply_visitor(eval_visitor(env), args.tail[0].tail[0].head);
            if (const int *p_offset = boost::get<int>(&offset_obj)) {
                offset = *p_offset;
            }
            else if (const double *p_offset = boost::get<double>(&offset_obj)){
                offset = *p_offset;
            }
        }

        if (offset < 0) {
            offset = 0;
        }

        if (offset > int(p_content->length())) {
            offset = p_content->length();
        }

        varlisp::List ret;

        if (p_reg->match(p_content->c_str() + offset)) {
            List * p_list = &ret;
            for (int i = 0; i < p_reg->submatch_count(); ++i) {
                p_list = p_list->next_slot();
                p_list->head = p_reg->submatch(i);
            }
        }

        return ret;
    }

    /**
     * @brief
     *      (regex-replace reg target fmt)->string
     *
     * @param [in]env
     * @param [in]args
     *
     * @return 
     */
    Object eval_regex_replace(varlisp::Environment& env, const varlisp::List& args)
    {
        Object reg_obj = boost::apply_visitor(eval_visitor(env), args.head);
        sss::regex::CRegex *p_reg = boost::get<sss::regex::CRegex>(&reg_obj);
        if (!p_reg) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-replace: regex obj");
        }

        Object target = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_content = boost::get<std::string>(&target);
        if (!p_content) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-replace: need one target string to replace");
        }

        Object fmt_obj = boost::apply_visitor(eval_visitor(env), args.tail[0].tail[0].head);
        const std::string *p_fmt = boost::get<std::string>(&fmt_obj);
        if (!p_fmt) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-replace: need one fmt string to replace");
        }

        std::string out; 
        p_reg->substitute(*p_content, *p_fmt, out);
        return out;
    }

    /**
     * @brief
     *      (regex-split sep-reg "target-string")
     *
     * @param [in] env
     * @param [in] args
     *
     * @return
     */
    Object eval_regex_split(varlisp::Environment& env, const varlisp::List& args)
    {
        Object reg_obj = boost::apply_visitor(eval_visitor(env), args.head);
        sss::regex::CRegex *p_reg = boost::get<sss::regex::CRegex>(&reg_obj);
        if (!p_reg) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-replace: regex obj");
        }

        Object target = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_content = boost::get<std::string>(&target);
        if (!p_content) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-replace: need one target string to replace");
        }

        const char * str_beg = p_content->c_str();
        varlisp::List ret;
        List * p_list = &ret;

        while (str_beg && *str_beg && p_reg->match(str_beg)) {
            p_list = p_list->next_slot();

            p_list->head = std::string(str_beg,
                                       str_beg + p_reg->submatch_start(0));

            str_beg += p_reg->submatch_end(0);
        }

        if (str_beg && *str_beg) {
            p_list = p_list->next_slot();
            p_list->head = std::string(str_beg);
        }

        return ret;
    }

    /**
     * @brief
     *      (regex-collect reg "target-string")
     *      (regex-collect reg "target-string" "fmt-string")
     *
     * @param [in] env
     * @param [in] args
     *
     * @return 
     */
    Object eval_regex_collect(varlisp::Environment& env, const varlisp::List& args)
    {
        Object reg_obj = boost::apply_visitor(eval_visitor(env), args.head);
        sss::regex::CRegex *p_reg = boost::get<sss::regex::CRegex>(&reg_obj);
        if (!p_reg) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-replace: regex obj");
        }

        Object target = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_content = boost::get<std::string>(&target);
        if (!p_content) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-replace: need one target string to replace");
        }
        const std::string *p_fmt = 0;
        if (args.length() == 3) {
            Object fmt_obj = boost::apply_visitor(eval_visitor(env), args.tail[0].tail[0].head);
            p_fmt = boost::get<std::string>(&fmt_obj);
        }

        const char * str_beg = p_content->c_str();
        varlisp::List ret;
        List * p_list = &ret;

        while (str_beg && *str_beg && p_reg->match(str_beg)) {
            p_list = p_list->next_slot();
            str_beg += p_reg->submatch_start(0);

            if (p_fmt) {
                std::ostringstream oss;
                for (std::string::size_type i = 0; i != p_fmt->length(); ++ i) {
                    if (p_fmt->at(i) == '\\' && i + 1 != p_fmt->length() && std::isdigit(p_fmt->at(i + 1))) {
                        int index = p_fmt->at(i + 1) - '0';
                        oss.write(str_beg + p_reg->submatch_start(index),
                                  p_reg->submatch_consumed(index));
                        ++i;
                    }
                    else {
                        oss << p_fmt->at(i);
                    }
                }
                p_list->head = oss.str();
            }
            else {
                p_list->head = std::string(str_beg,
                                           str_beg + p_reg->submatch_consumed(0));
            }
            str_beg += p_reg->submatch_consumed(0);
        }

        return ret;
    }

    /**
     * @brief
     *    (substr "target-string" offset)
     *    (substr "target-string" offset length)
     *
     * @param [in] env
     * @param [in] args
     *
     * @return 
     */
    Object eval_substr(varlisp::Environment& env, const varlisp::List& args)
    {
        Object target = boost::apply_visitor(eval_visitor(env), args.head);
        const std::string *p_content = boost::get<std::string>(&target);
        if (!p_content) {
            SSS_POSTION_THROW(std::runtime_error,
                              "regex-search: need one target string to search");
        }

        int offset = 0;
        Object offset_obj = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        if (const int *p_offset = boost::get<int>(&offset_obj)) {
            offset = *p_offset;
        }
        else if (const double *p_offset = boost::get<double>(&offset_obj)){
            offset = *p_offset;
        }

        if (offset < 0) {
            offset = 0;
        }
        if (offset > int(p_content->length())) {
            offset = p_content->length();
        }

        int length = -1;
        if (args.length() == 3) {
            Object length_obj = boost::apply_visitor(eval_visitor(env), args.tail[0].tail[0].head);
            if (const int *p_length = boost::get<int>(&length_obj)) {
                length = *p_length;
            }
            else if (const double *p_length = boost::get<double>(&length_obj)){
                length = *p_length;
            }
        }

        if (length < 0) {
            return p_content->substr(offset);
        }
        else {
            return p_content->substr(offset, length);
        }
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
            {"load",    1,  1, &eval_load},

            {"read",    1,  1, &eval_read},
            {"write",   2,  2, &eval_write},
            {"write-append", 2, 2, &eval_write_append},

            {"split",   1,  2, &eval_split},
            {"join",     1,  2, &eval_join},

            {"http-get",     1,  3, &eval_http_get},
            {"gumbo-query",  2,  2, &eval_gumbo_query},

            {"regex",           1,  1, &eval_regex},
            {"regex-match",     2,  2, &eval_regex_match},
            {"regex-search",    2,  3, &eval_regex_search},
            {"regex-replace",   3,  3, &eval_regex_replace},

            {"regex-split",     2,  2, &eval_regex_split},
            {"regex-collect",   2,  3, &eval_regex_collect},

            {"substr",   2, 3,  &eval_substr}
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
                              builtin_infos[m_type].name
                              << " need at least " << arg_min
                              << " parameters. but provided " << arg_length);
        }
        if (arg_max > 0 && arg_length > arg_max) {
            SSS_POSTION_THROW(std::runtime_error,
                              builtin_infos[m_type].name
                              << " need at most " << arg_max
                              << " parameters. but provided " << arg_length);
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
        env["load"] = varlisp::Builtin(varlisp::TYPE_LOAD);

        env["read"]     = varlisp::Builtin(varlisp::TYPE_READ);
        env["write"]    = varlisp::Builtin(varlisp::TYPE_WRITE);
        env["write-append"]    = varlisp::Builtin(varlisp::TYPE_WRITE_APPEND);

        env["split"]    = varlisp::Builtin(varlisp::TYPE_SPLIT);
        env["join"]     = varlisp::Builtin(varlisp::TYPE_JOIN);

        env["http-get"]     = varlisp::Builtin(varlisp::TYPE_HTTP_GET);
        env["gumbo-query"]  = varlisp::Builtin(varlisp::TYPE_GUMBO_QUERY);

        env["regex"]        = varlisp::Builtin(varlisp::TYPE_REGEX);
        env["regex-match"]  = varlisp::Builtin(varlisp::TYPE_REGEX_MATCH);
        env["regex-search"] = varlisp::Builtin(varlisp::TYPE_REGEX_SEARCH);
        env["regex-replace"]= varlisp::Builtin(varlisp::TYPE_REGEX_REPLACE);

        env["regex-split"]  = varlisp::Builtin(varlisp::TYPE_REGEX_SPLIT);
        env["regex-collect"]= varlisp::Builtin(varlisp::TYPE_REGEX_COLLECT);

        env["substr"]       = varlisp::Builtin(varlisp::TYPE_SUBSTR);
    }
} // namespace varlisp
