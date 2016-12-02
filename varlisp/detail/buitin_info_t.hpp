#include <vector>

#include "../object.hpp"
#include "../String.hpp"

namespace varlisp {
namespace detail {
struct Environment;
struct List;

typedef Object (*eval_func_t)(varlisp::Environment& env,
                              const varlisp::List& args);

// 参数格式；
// 闭区间；-1表示无穷
struct builtin_info_t {
    builtin_info_t(const char* name, int min, int max, eval_func_t func,
                   const char* help_msg)
        : name(name),
          min(min),
          max(max),
          eval_fun(func),
          help_msg(std::move(std::string(help_msg)))
    {}
    void params_size_check(int arg_len) const;
    const char *    name;
    int             min;
    int             max;
    eval_func_t     eval_fun;
    string_t        help_msg;
};

typedef std::vector<builtin_info_t> builtin_info_vet_t;
builtin_info_vet_t& get_builtin_infos();
inline bool regist_builtin_function(const char* name, int min, int max,
                                    eval_func_t func, const char * help_msg)
{
    get_builtin_infos().emplace_back(name, min, max, func, help_msg);
    return true;
}

#ifndef REGIST_BUILTIN
#define REGIST_BUILTIN(name, low, high, func, help_msg)          \
    Object func(varlisp::Environment&, const varlisp::List&);    \
    bool dummy##func = varlisp::detail::regist_builtin_function( \
        name, low, high, &func, help_msg);
#endif

} // namespace detail

} // namespace varlisp
