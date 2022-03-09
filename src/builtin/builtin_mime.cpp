// varlisp/builtin/builtin_mime.cpp
#include <array>

#include <re2/re2.h>

#include <sss/debug/value_msg.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/list_iterator.hpp"
#include "../detail/magic.hpp"

namespace varlisp {

REGIST_BUILTIN("mime-file", 1, 1, eval_mime_file,
               "; 基于系统mime database 算法，\n"
               "; 获得某文件的 mime-type 字符串描述\n"
               "(mime-file \"path-to-file\") -> mime-type-string");

/**
 * @brief (mime-file "path-to-file") -> mime-type-string
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_mime_file(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "mime-file";
    std::array<Object, 1> objs;
    const auto *p_file_name =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    static FileTyping::Magic mg;

    return string_t(mg.file(*p_file_name->gen_shared()));
}

REGIST_BUILTIN("mime-buffer", 1, 1, eval_mime_buffer,
               "; 基于系统mime database 算法，\n"
               "; 获得某buffer的 mime-type 字符串描述\n"
               "(mime-buffer \"string-buffer-to-detect\") -> mime-type-string");

/**
 * @brief (mime-buffer "string-buffer-to-detect") -> mime-type-string
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_mime_buffer(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "mime-buffer";
    std::array<Object, 1> objs;
    const auto *p_buffer =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    static FileTyping::Magic mg;
    auto p_s = p_buffer->gen_shared();
    return string_t(mg.buffer(p_s->data(), p_s->size()));
}

}  // namespace varlisp
