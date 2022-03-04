#include "builtin.hpp"

#include <fcntl.h>

#include <sss/colorlog.hpp>
#include <sss/util/PostionThrow.hpp>
#include <sss/log.hpp>
#include <sss/algorithm.hpp>

#include "environment.hpp"
#include "detail/buitin_info_t.hpp"

namespace varlisp {

typedef Object (*eval_func_t)(varlisp::Environment& env, const varlisp::List& args);

// 既然有常量的存在，那么我干脆……
// 将常亮孤立到额外的全局的map、vector变量中；
// 然后，让Environment，在按名字取Object对象的时候，
// 先询问这些全局对象。
void Builtin::regist_builtin_function(Environment& env)
{
    const std::vector<varlisp::detail::builtin_info_t>& info_vec
        = varlisp::detail::get_builtin_infos();
    for (size_t i = 0; i != info_vec.size() ; ++i) {
        env.insert(info_vec[i].name, varlisp::Builtin(i), true);
    }
#define CONSTANT_INT(i) (env.insert(#i, varlisp::Object{int64_t(i)}, true))
    CONSTANT_INT(O_RDONLY);
    CONSTANT_INT(O_WRONLY);
    CONSTANT_INT(O_RDWR);
    CONSTANT_INT(O_APPEND);
    CONSTANT_INT(O_TRUNC);
    CONSTANT_INT(O_CREAT);

    CONSTANT_INT(SEEK_CUR);
    CONSTANT_INT(SEEK_SET);
    CONSTANT_INT(SEEK_END);
#undef CONSTANT_INT

#define CONSTANT_INT2(name,i) (env.insert(#name, varlisp::Object{int64_t(i)}, true))
    CONSTANT_INT2(CL_NONE           ,sss::colog::ls_NONE);
    CONSTANT_INT2(CL_DATE           ,sss::colog::ls_DATE);
    CONSTANT_INT2(CL_TIME           ,sss::colog::ls_TIME);
    CONSTANT_INT2(CL_TIME_MILL      ,sss::colog::ls_TIME_MILL);
    CONSTANT_INT2(CL_TIME_MICR      ,sss::colog::ls_TIME_MICR);
    CONSTANT_INT2(CL_TIME_NANO      ,sss::colog::ls_TIME_NANO);
    CONSTANT_INT2(CL_TIME_MASK      ,sss::colog::ls_TIME_MASK);
    CONSTANT_INT2(CL_LEVEL          ,sss::colog::ls_LEVEL);
    CONSTANT_INT2(CL_LEVEL_SHORT    ,sss::colog::ls_LEVEL_SHORT);
    CONSTANT_INT2(CL_LEVEL_MASK     ,sss::colog::ls_LEVEL_MASK);
    CONSTANT_INT2(CL_FILE           ,sss::colog::ls_FILE);
    CONSTANT_INT2(CL_FILE_SHORT     ,sss::colog::ls_FILE_SHORT);
    CONSTANT_INT2(CL_FILE_VIM       ,sss::colog::ls_FILE_VIM);
    CONSTANT_INT2(CL_FILE_MASK      ,sss::colog::ls_FILE_MASK);
    CONSTANT_INT2(CL_LINE           ,sss::colog::ls_LINE);
    CONSTANT_INT2(CL_FUNC           ,sss::colog::ls_FUNC);
#undef CONSTANT_INT2
}

void Builtin::print(std::ostream& o) const
{
    o << "#<builtin:\""
      << varlisp::detail::get_builtin_infos()[this->m_type].name << "\">";
}

Builtin::Builtin(int type)
    : m_type(type)
{
}

varlisp::string_t Builtin::help_msg() const
{
    return varlisp::detail::get_builtin_infos()[this->type()].help_msg;
}

Object Builtin::eval(varlisp::Environment& env, const varlisp::List& args) const
{
    SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
    const varlisp::detail::builtin_info_vet_t&
        builtin_infos = varlisp::detail::get_builtin_infos();
    COLOG_DEBUG(builtin_infos[m_type].name, args);

    int arg_length = args.length();
    builtin_infos[m_type].params_size_check(arg_length);
    return builtin_infos[m_type].eval_fun(env, args);
}
} // namespace varlisp
