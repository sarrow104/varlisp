
#include "builtin.hpp"
#include "environment.hpp"
#include <sss/util/PostionThrow.hpp>
#include <sss/log.hpp>
#include <sss/algorithm.hpp>

namespace varlisp {

typedef Object (*eval_func_t)(varlisp::Environment& env, const varlisp::List& args);

// 参数格式；
// 闭区间；-1表示无穷
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

const builtin_info_t builtin_infos[] = 
{
    {"car",         1,  1, &eval_car},
    {"cdr",         1,  1, &eval_cdr},

    // 基本数学运算
    {"+",           1, -1, &eval_add},
    {"-",           1, -1, &eval_sub},
    {"*",           2, -1, &eval_mul},
    {"/",           2, -1, &eval_div},

    {"^",           2,  2, &eval_pow},

    // 逻辑运算
    {"=",           2,  2, &eval_eq},

    {">",           2,  2, &eval_gt},
    {"<",           2,  2, &eval_lt},
    {">=",          2,  2, &eval_ge},
    {"<=",          2,  2, &eval_le},

    // 执行
    {"eval",        1,  2, &eval_eval},
    {"load",        1,  1, &eval_load},

    // 文件读写
    {"read",        1,  1, &eval_read},
    {"write",       2,  2, &eval_write},
    {"write-append", 2, 2, &eval_write_append},

    // 字符串拆分，组合
    {"split",       1,  2, &eval_split},
    {"join",        1,  2, &eval_join},

    // 网络
    {"http-get",     1,  3, &eval_http_get},
    // html 解析
    {"gumbo-query",  2,  2, &eval_gumbo_query},

    // 正则表达式
    {"regex",           1,  1, &eval_regex},        // 从字符串生成regex对象
    {"regex-match",     2,  2, &eval_regex_match},  // 
    {"regex-search",    2,  3, &eval_regex_search},
    {"regex-replace",   3,  3, &eval_regex_replace},

    {"regex-split",     2,  2, &eval_regex_split},
    {"regex-collect",   2,  3, &eval_regex_collect},

    // 字符串-获取子串
    {"substr",      2,  3,  &eval_substr},

    {"shell",       1, -1,  &eval_shell}, // 至少一个参数；所有参数，讲组合成一个字符串，用来交给shell执行；然后返回输出
    {"shell-cd",    1,  1,  &eval_cd},    // 有且只能有一个参数；更改执行路径——如何显示在title？
    {"shell-ls",    0, -1,  &eval_ls},    // 允许任意个可以理解为路径的字符串作为参数；枚举出所有路径
    // NOTE 需要注意的是，就算是shell-ls本身，其实也是不支持通配符的！
    // 它看起来支持通配符，是因为'*'等符号，在交给ls之前，就已经被"expand"了！
    {"shell-pwd",   0,  0,  &eval_pwd},   // 无参数；打印当前路径

    {"fnamemodify", 2,  2,  &eval_fnamemodify}, // 有且只能两个参数；分别是路径和修改字符串；

    {"glob",        1,  2,  &eval_glob},  // 支持1到2个参数；分别是枚举路径和目标规则(可选)；
    {"glob-recurse",1,  3,  &eval_glob_recurse}, // 参数同上；第三个可选参数，指查找深度；
};


void Builtin::regist_builtin_function(Environment& env)
{
    for (size_t i = 0; i < sss::size(builtin_infos); ++i) {
        env[builtin_infos[i].name] = varlisp::Builtin(i);
    }
}

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
        SSS_POSTION_THROW(std::runtime_error, builtin_infos[m_type].name,
                          " need at least ", arg_min,
                          " parameters. but provided ", arg_length);
    }
    if (arg_max > 0 && arg_length > arg_max) {
        SSS_POSTION_THROW(std::runtime_error, builtin_infos[m_type].name,
                          " need at most ", arg_max,
                          " parameters. but provided ", arg_length);
    }
    return builtin_infos[m_type].eval_fun(env, args);
}
} // namespace varlisp
