#include "builtin.hpp"
#include "environment.hpp"

#include <sss/util/PostionThrow.hpp>
#include <sss/log.hpp>

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

typedef Object (*eval_func_t)(varlisp::Environment& env, const varlisp::List& args);

// ������ʽ��
// �����䣻-1��ʾ����
struct builtin_info_t {
    const char *    name;
    int             min;
    int             max;
    eval_func_t     eval_fun;
};

Object eval_car(varlisp::Environment& env, const varlisp::List& args);
Object eval_cdr(varlisp::Environment& env, const varlisp::List& args);

Object eval_add(varlisp::Environment& env, const varlisp::List& args);
Object eval_sub(varlisp::Environment& env, const varlisp::List& args);
Object eval_mul(varlisp::Environment& env, const varlisp::List& args);
Object eval_div(varlisp::Environment& env, const varlisp::List& args);
Object eval_pow(varlisp::Environment& env, const varlisp::List& args);


Object eval_eq(varlisp::Environment& env, const varlisp::List& args);
Object eval_gt(varlisp::Environment& env, const varlisp::List& args);
Object eval_lt(varlisp::Environment& env, const varlisp::List& args);
Object eval_ge(varlisp::Environment& env, const varlisp::List& args);
Object eval_le(varlisp::Environment& env, const varlisp::List& args);

Object eval_eval(varlisp::Environment& env, const varlisp::List& args);
Object eval_load(varlisp::Environment& env, const varlisp::List& args);

Object eval_read(varlisp::Environment& env, const varlisp::List& args);
Object eval_write(varlisp::Environment& env, const varlisp::List& args);
Object eval_write_append(varlisp::Environment& env, const varlisp::List& args);
Object eval_split(varlisp::Environment& env, const varlisp::List& args);
Object eval_join(varlisp::Environment& env, const varlisp::List& args);
Object eval_http_get(varlisp::Environment& env, const varlisp::List& args);
Object eval_gumbo_query(varlisp::Environment& env, const varlisp::List& args);
Object eval_regex(varlisp::Environment& env, const varlisp::List& args);
Object eval_regex_match(varlisp::Environment& env, const varlisp::List& args);
Object eval_regex_search(varlisp::Environment& env, const varlisp::List& args);
Object eval_regex_replace(varlisp::Environment& env, const varlisp::List& args);
Object eval_regex_split(varlisp::Environment& env, const varlisp::List& args);
Object eval_regex_collect(varlisp::Environment& env, const varlisp::List& args);
Object eval_substr(varlisp::Environment& env, const varlisp::List& args);
Object eval_shell(varlisp::Environment& env, const varlisp::List& args);
Object eval_cd(varlisp::Environment& env, const varlisp::List& args);
Object eval_ls(varlisp::Environment& env, const varlisp::List& args);
Object eval_pwd(varlisp::Environment& env, const varlisp::List& args);
Object eval_fnamemodify(varlisp::Environment& env, const varlisp::List& args);
Object eval_glob(varlisp::Environment& env, const varlisp::List& args);
Object eval_glob_recurse(varlisp::Environment& env, const varlisp::List& args);

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
