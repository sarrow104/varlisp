
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

Object eval_cons(varlisp::Environment& env, const varlisp::List& args);
Object eval_car(varlisp::Environment& env, const varlisp::List& args);
Object eval_cdr(varlisp::Environment& env, const varlisp::List& args);
Object eval_car_nth(varlisp::Environment& env, const varlisp::List& args);
Object eval_cdr_nth(varlisp::Environment& env, const varlisp::List& args);
Object eval_length(varlisp::Environment& env, const varlisp::List& args);
Object eval_append(varlisp::Environment& env, const varlisp::List& args);

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

Object eval_not(varlisp::Environment& env, const varlisp::List& args);
Object eval_null(varlisp::Environment& env, const varlisp::List& args);
Object eval_equal(varlisp::Environment& env, const varlisp::List& args);

Object eval_eval(varlisp::Environment& env, const varlisp::List& args);
Object eval_load(varlisp::Environment& env, const varlisp::List& args);

Object eval_read(varlisp::Environment& env, const varlisp::List& args);
Object eval_write(varlisp::Environment& env, const varlisp::List& args);
Object eval_write_append(varlisp::Environment& env, const varlisp::List& args);

Object eval_split(varlisp::Environment& env, const varlisp::List& args);
Object eval_join(varlisp::Environment& env, const varlisp::List& args);

Object eval_http_get(varlisp::Environment& env, const varlisp::List& args);
Object eval_gumbo(varlisp::Environment& env, const varlisp::List& args);
Object eval_gumbo_query(varlisp::Environment& env, const varlisp::List& args);
Object eval_gqnode_attr(varlisp::Environment& env, const varlisp::List& args);
Object eval_gqnode_hasAttr(varlisp::Environment& env, const varlisp::List& args);
Object eval_gqnode_valid(varlisp::Environment& env, const varlisp::List& args);
Object eval_gqnode_text(varlisp::Environment& env, const varlisp::List& args);
Object eval_gqnode_textNeat(varlisp::Environment& env, const varlisp::List& args);
Object eval_gqnode_ownText(varlisp::Environment& env, const varlisp::List& args);
Object eval_gqnode_tag(varlisp::Environment& env, const varlisp::List& args);
Object eval_gqnode_isText(varlisp::Environment& env, const varlisp::List& args);
Object eval_gqnode_innerHtml(varlisp::Environment& env, const varlisp::List& args);
Object eval_gqnode_outerHtml(varlisp::Environment& env, const varlisp::List& args);
Object eval_gumbo_query_text(varlisp::Environment& env, const varlisp::List& args);

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

Object eval_map(varlisp::Environment &env, const varlisp::List &args);
Object eval_reduce(varlisp::Environment &env, const varlisp::List &args);
Object eval_filter(varlisp::Environment &env, const varlisp::List &args);

Object eval_uchardet(varlisp::Environment& env, const varlisp::List& args);
Object eval_pychardet(varlisp::Environment& env, const varlisp::List& args);
Object eval_ivchardet(varlisp::Environment& env, const varlisp::List& args);
Object eval_iconv(varlisp::Environment& env, const varlisp::List& args);
Object eval_ensure_utf8(varlisp::Environment& env, const varlisp::List& args);

const builtin_info_t builtin_infos[] =
{
    {"cons",            2,  2, &eval_cons},
    {"car",             1,  1, &eval_car},
    {"cdr",             1,  1, &eval_cdr},
    {"car-nth",         2,  2, &eval_car_nth},
    {"cdr-nth",         2,  2, &eval_cdr_nth},
    {"length",          1,  1, &eval_length},
    {"append",          2,  2, &eval_append},

    // 基本数学运算
    {"+",               1, -1, &eval_add},
    {"-",               1, -1, &eval_sub},
    {"*",               2, -1, &eval_mul},
    {"/",               2, -1, &eval_div},

    {"^",               2,  2, &eval_pow},

    // 逻辑运算
    {"=",               2,  2, &eval_eq},

    {">",               2,  2, &eval_gt},
    {"<",               2,  2, &eval_lt},
    {">=",              2,  2, &eval_ge},
    {"<=",              2,  2, &eval_le},

    {"not",             1,  1, &eval_not},
    {"null",            1,  1, &eval_null},
    {"equal",           2,  2, &eval_equal},

    // 执行
    {"eval",            1,  2, &eval_eval},
    {"load",            1,  1, &eval_load},

    // 文件读写
    {"read",            1,  1, &eval_read},
    {"write",           2,  2, &eval_write},
    {"write-append",    2,  2, &eval_write_append},
    // maybe write-ln

    // 字符串拆分，组合
    {"split",           1,  2, &eval_split},
    {"join",            1,  2, &eval_join},

    // 网络
    {"http-get",        1,  3, &eval_http_get},

    {"gumbo",           1,  2, &eval_gumbo},
    // html 解析
    {"gumbo-query",     2,  2, &eval_gumbo_query},

    {"gqnode:attr",     2,  2, &eval_gqnode_attr},
    {"gqnode:hasAttr",  2,  2, &eval_gqnode_hasAttr},
    {"gqnode:valid",    1,  1, &eval_gqnode_valid},
    {"gqnode:text",     1,  1, &eval_gqnode_text},
    {"gqnode:textNeat", 1,  1, &eval_gqnode_textNeat},
    {"gqnode:ownText",  1,  1, &eval_gqnode_ownText},
    {"gqnode:tag",      1,  1, &eval_gqnode_tag},
    {"gqnode:isText",   1,  1, &eval_gqnode_isText},
    {"gqnode:innerHtml",1,  1, &eval_gqnode_innerHtml},
    {"gqnode:outerHtml",1,  1, &eval_gqnode_outerHtml},

    {"gumbo-query-text",2,  2, &eval_gumbo_query_text},

    // 正则表达式
    {"regex",           1,  1, &eval_regex},        // 从字符串生成regex对象
    {"regex-match",     2,  2, &eval_regex_match},  //
    {"regex-search",    2,  3, &eval_regex_search},
    {"regex-replace",   3,  3, &eval_regex_replace},

    {"regex-split",     2,  2, &eval_regex_split},
    {"regex-collect",   2,  3, &eval_regex_collect},

    // 字符串-获取子串
    {"substr",      2,  3,  &eval_substr},

    {"shell",       1, -1,  &eval_shell}, // 至少一个参数；所有参数，将组合成一个字符串，用来交给shell执行；然后返回输出
    {"shell-cd",    1,  1,  &eval_cd},    // 有且只能有一个参数；更改执行路径——如何显示在title？
    {"shell-ls",    0, -1,  &eval_ls},    // 允许任意个可以理解为路径的字符串作为参数；枚举出所有路径
    // NOTE 需要注意的是，就算是shell-ls本身，其实也是不支持通配符的！
    // 它看起来支持通配符，是因为'*'等符号，在交给ls之前，就已经被"expand"了！
    {"shell-pwd",   0,  0,  &eval_pwd},   // 无参数；打印当前路径

    {"fnamemodify", 2,  2,  &eval_fnamemodify}, // 有且只能两个参数；分别是路径和修改字符串；

    {"glob",        1,  2,  &eval_glob},  // 支持1到2个参数；分别是枚举路径和目标规则(可选)；
    {"glob-recurse",1,  3,  &eval_glob_recurse}, // 参数同上；第三个可选参数，指查找深度；

    // ;;(map list-type function list-1 list-2 ... list-n)
    // ;;map函数接受一个函数和N个列表，该函数接受N个参数；
    // 返回一个列表。返回列表的每个元素都是使用输入的函数
    // 对N个类别中的每个元素处理的结果。
    {"map",         2,  -1, &eval_map},

    // ;;(reduce function list)
    // ;;reduce让一个指定的函数(function)作用于列表的第一个
    // 元素和第二个元素,然后在作用于上步得到的结果和第三个
    // 元素，直到处理完列表中所有元素。
    {"reduce",      2,  2,  &eval_reduce},

    // filter
    {"filter",      2,  2,  &eval_filter},

    {"uchardet",    1,  1,  &eval_uchardet},
    {"pychardet",   1,  1,  &eval_pychardet},
    {"ivchardet",   2,  2,  &eval_ivchardet},
    {"iconv",       3,  3,  &eval_iconv},
    {"ensure-utf8", 2,  3,  &eval_ensure_utf8},
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
