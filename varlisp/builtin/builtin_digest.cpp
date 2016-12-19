#include <sss/path.hpp>
#include <sss/utlstring.hpp>

#include <ss1x/uuid/sha1.hpp>

#include "../builtin_helper.hpp"
#include "../object.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../environment.hpp"

namespace varlisp {

REGIST_BUILTIN("digest-sha1-file", 1, 1, eval_digest_sha1_file,
               "(digest-sha1-file \"path/to/file\") -> \"sha1-string\" | nil");

Object eval_digest_sha1_file(varlisp::Environment& env,
                             const varlisp::List& args)
{
    const char* funcName = "digest-sha1-file";
    Object tmp;
    const auto* p_fname = varlisp::requireTypedValue<varlisp::string_t>(
        env, detail::car(args), tmp, funcName, 0, DEBUG_INFO);
    try {
        std::string path = sss::path::full_of_copy(p_fname->to_string());
        return string_t(ss1x::uuid::sha1::fromFile(path));
    }
    catch (...) {
        return varlisp::Nill{};
    }
}

REGIST_BUILTIN(
    "digest-sha1-string", 1, 1, eval_digest_sha1_string,
    "(digest-sha1-string \"bytes-content-to-sha1\") -> \"sha1-string\"");

Object eval_digest_sha1_string(varlisp::Environment& env,
                               const varlisp::List& args)
{
    const char* funcName = "digest-sha1-string";
    Object tmp;
    const auto* p_string = varlisp::requireTypedValue<varlisp::string_t>(
        env, detail::car(args), tmp, funcName, 0, DEBUG_INFO);

    return string_t(
        ss1x::uuid::sha1::fromBytes(p_string->data(), p_string->size()));
}

REGIST_BUILTIN(
    "digest-hex-string", 1, 1, eval_digest_hex_string,
    "(digest-hex-string \"bytes-content-to-hex\") -> \"hex-string\"");

Object eval_digest_hex_string(varlisp::Environment& env,
                              const varlisp::List& args)
{
    const char* funcName = "digest-sha1-string";
    Object tmp;
    const auto* p_string = varlisp::requireTypedValue<varlisp::string_t>(
        env, detail::car(args), tmp, funcName, 0, DEBUG_INFO);

    return string_t(sss::to_hex(p_string->to_string()));
}

}  // namespace varlisp
