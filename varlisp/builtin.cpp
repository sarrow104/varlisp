#include <cmath>
#include <sstream>
#include <memory>

#include <sss/util/PostionThrow.hpp>
#include <sss/log.hpp>
#include <sss/iConvpp.hpp>
#include <sss/encoding.hpp>
#include <sss/path.hpp>
#include <sss/path/name_filter.hpp>
#include <sss/path/glob_path.hpp>
#include <sss/path/glob_path_recursive.hpp>

#include <ss1x/asio/GetFile.hpp>
#include <ss1x/asio/headers.hpp>
#include <ss1x/asio/utility.hpp>

#include <sss/spliter.hpp>
#include <sss/colorlog.hpp>

#include "raw_stream_visitor.hpp"
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
    TYPE_CAR,
    TYPE_CDR,

    // ������ѧ����
    TYPE_ADD,
    TYPE_SUB,
    TYPE_MUL,
    TYPE_DIV,

    TYPE_POW,

    // �߼�����
    TYPE_EQ,

    TYPE_GT,
    TYPE_LT,

    TYPE_GE,
    TYPE_LE,

    // ִ��
    TYPE_EVAL,
    TYPE_LOAD,

    // �ļ���д
    TYPE_READ,
    TYPE_WRITE,
    TYPE_WRITE_APPEND,

    // �ַ�����֣����
    TYPE_SPLIT,
    TYPE_JOIN,

    // ����
    TYPE_HTTP_GET,
    // html ����
    TYPE_GUMBO_QUERY,

    // ������ʽ
    TYPE_REGEX,
    TYPE_REGEX_MATCH,
    TYPE_REGEX_SEARCH,
    TYPE_REGEX_REPLACE,

    TYPE_REGEX_SPLIT,
    TYPE_REGEX_COLLECT,

    // �ַ���-��ȡ�Ӵ�
    TYPE_SUBSTR,

    TYPE_SHELL, // ִ��shell�Ų�
    TYPE_SHELL_CD, // ����ִ��·��
    TYPE_SHELL_LS, // ö���ļ�
    TYPE_SHELL_PWD, // ��ȡ��ǰ·��
    TYPE_FNAMEMODIFY, // �޸�·��������

    TYPE_GLOB,          // ö�ٷ��Ϲ�����ļ��������
    TYPE_GLOB_RECURSE,  // ö�ٷ��Ϲ�����ļ��������
};

void Builtin::regist_builtin_function(Environment& env)
{
    env["car"]  = varlisp::Builtin(varlisp::TYPE_CAR);
    env["cdr"]  = varlisp::Builtin(varlisp::TYPE_CDR);

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

    env["shell"]        = varlisp::Builtin(varlisp::TYPE_SHELL);
    env["shell-cd"]     = varlisp::Builtin(varlisp::TYPE_SHELL_CD);
    env["shell-ls"]     = varlisp::Builtin(varlisp::TYPE_SHELL_LS);
    env["shell-pwd"]    = varlisp::Builtin(varlisp::TYPE_SHELL_PWD);
    env["fnamemodify"]  = varlisp::Builtin(varlisp::TYPE_FNAMEMODIFY);

    env["glob"]         = varlisp::Builtin(varlisp::TYPE_GLOB);
    env["glob-recurse"] = varlisp::Builtin(varlisp::TYPE_GLOB_RECURSE);
}

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

const varlisp::List * getFirstListPtrFromArg(varlisp::Environment& env, const varlisp::List& args, Object& tmp)
{
    // NOTE List �ĵ�һ��Ԫ����symbol ��list!
    const varlisp::List * _list = boost::get<varlisp::List>(&(args.head));
    if (!_list) {
        // list����������һ��`list`��symbol����˴���������ͬ��
        tmp = boost::apply_visitor(eval_visitor(env), args.head);
        _list = boost::get<varlisp::List>(&tmp);
    }
    else {
        // descard `list` symbol
        _list = _list->next();
    }
    return _list;
}

/**
 * @brief (car (list item1 item2 ...)) -> item1
 *
 * @param env
 * @param args
 *
 * @return 
 */
Object eval_car(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj;
    const varlisp::List * _list = getFirstListPtrFromArg(env, args, obj);

    if (!_list) {
        SSS_POSTION_THROW(std::runtime_error,
                          "(car: need List)");
    }

    if (!_list->length()) {
        SSS_POSTION_THROW(std::runtime_error,
                          "(car: contract violation expected: pair?  given: '())");
    }
    return _list->head;
}

/**
 * @brief (cdr (list item1 item2 ...)) -> (item2 item3 ...)
 *
 * @param env
 * @param args
 *
 * @return 
 *
 * TODO FIXME �����������Ҫ����eval_car���졣
 */
Object eval_cdr(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj;
    const varlisp::List * _list = getFirstListPtrFromArg(env, args, obj);

    if (!_list) {
        SSS_POSTION_THROW(std::runtime_error,
                          "(cdr: need List)");
    }
    if (!_list->length()) {
        SSS_POSTION_THROW(std::runtime_error,
                          "(cdr: contract violation expected: pair?  given: '())");
    }
    varlisp::List ret;
    List * p_list = &ret;

    _list = _list->next(); // descard the first object
    while (_list && _list->head.which()) {
        p_list = p_list->next_slot();
        p_list->head = _list->head;
        _list = _list->next();
    }
    return ret;
}

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

// ����drracket��˵���Ƚ������������ֱ��Ҫ��ת����real��
// ���ʧ�ܣ����׳��쳣��
// ����lambda��ʽ����1.2ʵ���ıȽϴ�С��������׳��쳣��
// > (if (> fib 1) 1 2)
// >: contract violation
//   expected: real?
//   given: #<procedure:fib>
//   argument position: 1st
//   other arguments...:
//    1
// ���У�
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
        // ���ڻ������������Ը��ݴ��������symbol��Ȼ��ͨ��ȫ�ֺ�������ȡ��
        // �����������ã�Ȼ��Ӧ�õ����
    }
    // ��Ҫ�ȶԲ���evalһ�Σ�Ȼ����evalһ�Σ�
    // Ϊʲô��Ҫ���Σ�
    // ��Ϊ����Ȼ��ͨ��eval���ý����ģ���ô�����eval������lisp���﷨������
    // �Ƚ���()�ڲ���Ȼ����Ϊ�ⲿ�Ĳ����ٴν�����
    //
    // ���ԣ�������eval�������ֱ���ԭ�е��û������������eval�ؼ����Լ������Ķ�����
    varlisp::Object first_res = boost::apply_visitor(eval_visitor(env), args.head);
    return boost::apply_visitor(eval_visitor(env), first_res);
}

// TODO
Object eval_load(varlisp::Environment& env, const varlisp::List& args)
{
    // NOTE ��Ȼ load���ڽ���������ôload���Ϳ��Գ������κεط�����������һ���ű���
    // �����ҵĽ����������ڽű���load�������ϣ������ڲ���Tokenizer����
    // push��popһ������Ĺ���ջ�������ǣ���ǰ��ֻ�ǣ�Tokenizerֻӵ����������ջ���൱��˫��������
    // ����Ƕ��load��ʹ�ã�����ܻ��������ģ�
    //
    // ���ǣ���ȫ�����������֣�
    //      1. ��Tokenizer���󣬳�Ϊ�����Ķ�ջ�Ķ���
    //      2. ����Ҫ����ʱ����Tokenizer�����Թ������á����൱������Tokenizer�Ķ�ջ��
    //
    // NOTE ���⣬��load��ʱ���Ƿ���Ҫ�½�һ��Environment�����أ�
    // ��������������⣻��������(load "path/to/script")�����ԣ��½�����
    // �ı�ʶ��(���󡢺����ȵ�)��Ӧ�÷ŵ��ĸ�Environment���أ�

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
                         "path `", *p_path, "` not to file");
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
   COLOG_INFO("(load ", sss::raw_string(*p_path), " complete)");
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
                          "path `" , *p_path , "` not to file");
    }
    // varlisp::List content;
    // std::string line;
    std::string content;
    sss::path::file2string(full_path, content);

    // ����С�ļ������жϿ��ܳ���
    // ensure_utf8(content, "cp936,utf8");

    return content;
}

/**
 * @brief eval_write_impl 
 *           (write        (list) path)
 *           (write-append (list) path)
 *           (write        item path)
 *           (write-append item path)
 *
 * ԭ�����޸�ʽ��Ҳû�ж������"sep-string"
 *
 * @param env
 * @param args
 * @param append
 *
 * @return 
 */
Object eval_write_impl(varlisp::Environment& env, const varlisp::List& args, bool append)
{
    Object obj;
    const varlisp::List * p_list = getFirstListPtrFromArg(env, args, obj);

    Object path = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    const std::string *p_path = boost::get<std::string>(&path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error,
                          "(write: requies path to write)");
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
                          "(write: failed open file ", sss::raw_string(*p_path), " to write");
    }

    std::ofstream::pos_type pos = ofs.tellp();
    if (p_list) {
        while (p_list && p_list->head.which()) {
            boost::apply_visitor(raw_stream_visitor(ofs, env), p_list->head);
            p_list = p_list->next();
        }
    }
    else {
        boost::apply_visitor(raw_stream_visitor(ofs, env), args.head);
    }

    std::ofstream::pos_type write_cnt = ofs.tellp() - pos;

    if (append) {
        COLOG_INFO("(write-append ", sss::raw_string(*p_path), " by ", int(write_cnt), "bytes complete)");
    }
    else {
        COLOG_INFO("(write ", sss::raw_string(*p_path), "by ", int(write_cnt), "bytes complete)");
    }
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
 * @brief ����ַ���
 *
 * @param [in]env
 * @param [in]args ֧���������������ֱ���з��ַ������ͷָ��ַ�����
 *
 * @return �ָ�֮����б�
 *
 * TODO ֧��������ʽ��ȷ��sep!
 * ��Ҫ����������
 * ���е����������Ǳ�ʾ����ĵ�symbol
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
 * @param [in] args ��һ��������������һ��(list)������symbol
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
        Object obj;
        if (!p_stem) {
            obj = boost::apply_visitor(eval_visitor(env), p_list->head);
            p_stem = boost::get<std::string>(&obj);
            if (!p_stem) {
                break;
            }
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

// TODO
// ����ʧ�ܵ����أ�Ӧ�ø�֪�û�content-length���Լ���ֹ�ںδ�(�Ѿ����ܵ�bytes��)
// ���⣬ensure-utf��Ӧ�ý����û����������Զ���ɡ�
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
    std::string charset = sss::trim_copy(headers.get("Content-Type", "charset"));
    if (!charset.empty()) {
        COLOG_INFO("(http-get: charset from Content-Type = ", sss::raw_string(charset), ")");
    }
    if (charset.empty()) {
        charset = "gb2312,utf8";
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

/**
 * @brief (regex "regex-string") -> regex-obj
 *
 * @param env
 * @param args
 *
 * @return 
 */
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

/**
 * @brief (regex-match reg-obj target-string) -> bool
 *
 * @param env
 * @param args
 *
 * @return 
 */
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
 * @brief (regex-search reg target offset = 0) -> (list sub0, sub1...)
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
 *      (regex-replace reg-obj target fmt)->string
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
 *      (regex-split sep-reg "target-string") -> (list stem1 stem2 ...)
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
 *          -> (list matched-sub1 matched-sub2 ...)
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
 *      -> sub-str
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

// Ӧ����δ���shell-evalʱ��Ĳ�����
// ���磬�û������ṩһ���������������ַ�����������ִ�е�����Լ�������
// ��ʱ�������������أ��������ţ�Ȼ��ִ�У����군�ˡ�
//
// ͬʱ��Ҳ�п�����Ҫִ�е�����Լ��ⲿ�����Ĳ������Ƿֿ��ġ����磬
// ����һ���������Ǵ���·�����ַ��������һ��пո���ô��������ţ�
// ���߽���ת�壬����һ������Ķ����ˡ�
//
// ����Ȼ���������ֲ����ݵĹ�����ʽ��
Object eval_shell(varlisp::Environment& env, const varlisp::List& args)
{
    // TODO
    return Object{};
}
Object eval_cd(varlisp::Environment& env, const varlisp::List& args)
{
    Object target_path = boost::apply_visitor(eval_visitor(env), args.head);
    const std::string *p_path = boost::get<std::string>(&target_path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error,
                          "shell-cd: requie one path string!");
    }
    sss::path::chgcwd(*p_path);
    COLOG_INFO("(shell-cd: ", sss::raw_string(*p_path), " complete)");
    return Object(sss::path::getcwd());
}
// {"ls",          0, -1,  &eval_ls},      // ����������������Ϊ·�����ַ�����Ϊ������ö�ٳ�����·��
Object eval_ls(varlisp::Environment& env, const varlisp::List& args)
{
    varlisp::List ret;
    const List * p = &args;
    List * p_list = &ret;
    if (args.length()) {
        while (p && p->head.which()) {
            Object ls_arg = boost::apply_visitor(eval_visitor(env), p->head);
            const std::string *p_ls_arg = boost::get<std::string>(&ls_arg);
            if (!p_ls_arg) {
                SSS_POSTION_THROW(std::runtime_error, "shell-ls: require string-type args");
            }
            switch (sss::path::file_exists(*p_ls_arg)) {
                case sss::PATH_TO_FILE:
                    p_list = p_list->next_slot();
                    p_list->head = *p_ls_arg;
                    break;

                case sss::PATH_TO_DIRECTORY:
                    {
                        sss::path::file_descriptor fd;
                        sss::path::glob_path gp(*p_ls_arg, fd);
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
                    break;

                case sss::PATH_NOT_EXIST:
                    COLOG_ERROR("(shell-ls: path", sss::raw_string(*p_ls_arg), "not exists)");
                    break;
            }
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
    }
    else {
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

Object eval_pwd(varlisp::Environment& env, const varlisp::List& args)
{
    (void)env;
    (void)args;
    return Object(sss::path::getcwd());
}
Object eval_fnamemodify(varlisp::Environment& env, const varlisp::List& args)
{
    Object path = boost::apply_visitor(eval_visitor(env), args.head);

    const std::string *p_path = boost::get<std::string>(&path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error,
                          "fnamemodify requies one path string");
    }
    Object modifier = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
    const std::string *p_modifier = boost::get<std::string>(&modifier);
    if (!p_modifier) {
        SSS_POSTION_THROW(std::runtime_error,
                          "fnamemodify requies one path-modifier string");
    }
    return Object(sss::path::modify_copy(*p_path, *p_modifier));
}

// {"glob",        1,  2,  &eval_glob}, // ֧��1��2���������ֱ���ö��·����Ŀ�����(��ѡ)��
Object eval_glob(varlisp::Environment& env, const varlisp::List& args)
{
    Object path = boost::apply_visitor(eval_visitor(env), args.head);

    const std::string *p_path = boost::get<std::string>(&path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error,
                          "glob requies one path string");
    }
    std::unique_ptr<sss::path::filter_t> f;
    Object filter;
    if (args.length() > 1) {
        filter = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_filter = boost::get<std::string>(&filter);
        if (!p_filter) {
            SSS_POSTION_THROW(std::runtime_error,
                              "glob: second filter arg must be a string");
        }
        f.reset(new sss::path::name_filter_t(*p_filter));
    }

    varlisp::List ret;
    List * p_list = &ret;

    sss::path::file_descriptor fd;
    sss::path::glob_path gp(".", fd, f.get());
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

    return Object(ret);
}

// {"glob-recurse", 1,  3,  &eval_glob_recurse}, // ����ͬ�ϣ���������ѡ������ָ������ȣ�
Object eval_glob_recurse(varlisp::Environment& env, const varlisp::List& args)
{
    Object path = boost::apply_visitor(eval_visitor(env), args.head);

    const std::string *p_path = boost::get<std::string>(&path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error,
                          "glob-recurse: requies one path string");
    }
    std::unique_ptr<sss::path::filter_t> f;
    if (args.length() > 1) {
        Object arg = boost::apply_visitor(eval_visitor(env), args.tail[0].head);
        const std::string *p_filter = boost::get<std::string>(&arg);
        if (!p_filter) {
            SSS_POSTION_THROW(std::runtime_error,
                              "glob-recurse: second filter arg must be a string");
        }
        f.reset(new sss::path::name_filter_t(*p_filter));
    }

    int depth = 0;
    if (args.length() > 2) {
        Object arg = boost::apply_visitor(eval_visitor(env), args.tail[0].tail[0].head);
        const int *p_depth = boost::get<int>(&arg);
        if (!p_depth) {
            SSS_POSTION_THROW(std::runtime_error,
                              "glob-recurse: third arg must be an integar");
        }
        depth = *p_depth;
    }

    varlisp::List ret;
    List * p_list = &ret;

    sss::path::file_descriptor fd;
    sss::path::glob_path_recursive gp(*p_path, fd, f.get(), false);
    gp.max_depth(depth);
    while (gp.fetch()) {
        if (fd.is_normal_file()) {
            p_list = p_list->next_slot();
            p_list->head = std::string(sss::path::relative_to(fd.get_path(), *p_path));
        }
    }

    return Object(ret);
}

// ������ʽ��
// �����䣻-1��ʾ����
struct builtin_info_t {
    const char *    name;
    int             min;
    int             max;
    eval_func_t     eval_fun;
};

const builtin_info_t builtin_infos[]
    = {
        {"car",     1,  1, &eval_car},
        {"cdr",     1,  1, &eval_cdr},

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

        {"regex",           1,  1, &eval_regex},        // ���ַ�������regex����
        {"regex-match",     2,  2, &eval_regex_match},  // 
        {"regex-search",    2,  3, &eval_regex_search},
        {"regex-replace",   3,  3, &eval_regex_replace},

        {"regex-split",     2,  2, &eval_regex_split},
        {"regex-collect",   2,  3, &eval_regex_collect},

        {"substr",      2,  3,  &eval_substr},

        {"shell",       1, -1,  &eval_shell}, // ����һ�����������в���������ϳ�һ���ַ�������������shellִ�У�Ȼ�󷵻����
        {"cd",          1,  1,  &eval_cd},      // ����ֻ����һ������������ִ��·�����������ʾ��title��
        {"ls",          0, -1,  &eval_ls},      // ����������������Ϊ·�����ַ�����Ϊ������ö�ٳ�����·��
                                                // NOTE ��Ҫע����ǣ�������shell-ls������ʵҲ�ǲ�֧��ͨ����ģ�
                                                // ��������֧��ͨ���������Ϊ'*'�ȷ��ţ��ڽ���ls֮ǰ�����Ѿ���"expand"�ˣ�
        {"pwd",         0,  0,  &eval_pwd},     // �޲�������ӡ��ǰ·��
        {"fnamemodify", 2,  2,  &eval_fnamemodify}, // ����ֻ�������������ֱ���·�����޸��ַ�����

        {"glob",        1,  2,  &eval_glob}, // ֧��1��2���������ֱ���ö��·����Ŀ�����(��ѡ)��
        {"glob-recurse",1,  3,  &eval_glob_recurse}, // ����ͬ�ϣ���������ѡ������ָ������ȣ�
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
                          , " need at least " , arg_min
                          , " parameters. but provided " , arg_length);
    }
    if (arg_max > 0 && arg_length > arg_max) {
        SSS_POSTION_THROW(std::runtime_error,
                          builtin_infos[m_type].name
                          , " need at most " , arg_max
                          , " parameters. but provided " , arg_length);
    }
    return builtin_infos[m_type].eval_fun(env, args);
}
} // namespace varlisp
