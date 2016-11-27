#include "object.hpp"
#include "builtin_helper.hpp"

#include <uchardet/uchardet.h>
#include <cctype>

#include <sss/encoding.hpp>
#include <sss/colorlog.hpp>
#include <sss/utlstring.hpp>
#include <sss/spliter.hpp>
#include <sss/iConvpp.hpp>
#include <sss/ps.hpp>
#include <sss/popenRWE.h>

#include <stdexcept>

inline void encoding_normalize(std::string& encoding)
{
    sss::to_lower(encoding);
    // utf-8, utf-16be utf-32le
    if (sss::is_begin_with(encoding, "utf-") ||
        sss::is_begin_with(encoding, "ucs-")) {
        encoding.erase(encoding.begin() + 3);
    }
    if (encoding.empty() || encoding == "none") {
        encoding.assign("ascii");
    }
    // ucs-2     16 bit UCS-2 encoded Unicode (ISO/IEC 10646-1)
    // ucs-2le   like ucs-2, little endian
    // utf-16    ucs-2 extended with double-words for more characters
    // utf-16le  like utf-16, little endian
    // ucs-4     32 bit UCS-4 encoded Unicode (ISO/IEC 10646-1)
    // ucs-4le   like ucs-4, little endian
}

//   uchardet content
//   pychardet content
//   ivchardet "ascii,cp936,utf8" content -> "utf8
//   iconv "cp936" "utf8" content -> converted-content
//
namespace varlisp {

/**
 * @brief
 *      (uchardet "content") -> "utf8"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_uchardet(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj;
    const string_t * p_content = varlisp::getTypedValue<string_t>(env, args.head, obj);
    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(uchardet: 1st argument must be string)");
    }

    uchardet_t ud = uchardet_new();
    std::string encoding;
    int err = uchardet_handle_data(ud, p_content->c_str(), p_content->length());
    if (!err)
    {
        uchardet_data_end(ud);
        encoding = uchardet_get_charset(ud);
        encoding_normalize(encoding);
    }

    uchardet_delete(ud);

    if (encoding.empty()) {
        encoding.assign("ascii");
    }

    if (err) {
        SSS_POSITION_THROW(std::runtime_error,
                          "uchardet: analyze coding faild！");
    }
    return string_t(std::move(encoding));
}

/**
 * @brief
 *      (pychardet "content") -> "utf8"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_pychardet(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj;
    const string_t * p_content = varlisp::getTypedValue<string_t>(env, args.head, obj);
    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(pychardet: 1st argument must be string)");
    }
    // Python 支持两种使用方式：
    //
    // 1. 外部文件；
    // 2. stdin读取；
    //
    // 前者，不一定适用；因为字符不一定来自外存；
    // 后者的麻烦在于，需要实现双向管道！
    int rwepipe_data[3] = {0, 0, 0};
    int pid = popenRWE(rwepipe_data, "chardet");
    if (pid == -1) {
        return "";
    }
    // TODO
    // 只写入至多1000行；貌似，这是不可能完成的任务！
    //
    // 这是因为，如果是没有'\0'字节的还好，判断'\n'，就能分辨出，是否过了一行；
    // 如果是有'\0'的，就说明是ucs2、ucs4这种编码——意味着，需要判断“字”
    // 的长度，才方便传入合适的字节数；
    //
    // 不然，如果在某一个“字”的中间，发生了切断，很可能导致判断出错；
    //
    // 当然，这种情况下，传入可以被4整除的字节数是一个安全的作法；
    //
    // 不定长的编码，那么，判断行（或者任意一个ascii字符），为结尾，比较安全；
    //
    // 那么，麻烦了——这是一个死循环；因为ucs2，将很难与不定长类的编码，区分开！
    size_t w_cnt = write(rwepipe_data[0], p_content->data(), p_content->size());
    if (w_cnt != p_content->size()) {
        return Object{Nill{}};
    }
    close(rwepipe_data[0]);
    rwepipe_data[0] = -1;
    char out_buf[1024];
    size_t cnt = read(rwepipe_data[1], out_buf, sizeof(out_buf));
    out_buf[cnt] = '\0';
    pcloseRWE(pid, rwepipe_data);
    if (!sss::is_begin_with(out_buf, "<stdin>: ")) {
        return Object{Nill{}};
    }
    char* next_space = std::strchr(out_buf + 9, ' ');
    if (next_space) {
        next_space[0] = '\0';
        std::string encoding(out_buf + 9);

        encoding_normalize(encoding);
        return string_t(std::move(encoding));
    }
    return Object{Nill{}};
}

namespace detail {
void trim(sss::string_view& s)
{
    while(s.size() && std::isspace(s.front())) {
        s.remove_prefix(1);
    }
    while(s.size() && std::isspace(s.back())) {
        s.remove_suffix(1);
    }
}

} // namespace detail

/**
 * @brief
 *      (ivchardet "encodings" "content") -> "utf8"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_ivchardet(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const string_t * p_encodings = varlisp::getTypedValue<string_t>(env, args.head, obj1);
    if (!p_encodings) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(ivchardet: 1st argument must be encodings string)");
    }
    Object obj2;
    const string_t * p_content = varlisp::getTypedValue<string_t>(env, args.tail[0].head, obj2);
    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(ivchardet: 2nd argument must be content string)");
    }

    sss::string_view encoding;
    bool has_found = false;
    sss::ViewSpliter<char> sp(*p_encodings, ',');
    std::string out;
    while (sp.fetch_next(encoding)) {
        detail::trim(encoding);
        try {
            std::string encoding_str = encoding.to_string();
            sss::iConv ic(encoding_str, encoding_str);
            if (!ic.is_ok()) {
                continue;
            }
            if (ic.convert(out, p_content->to_string())) {
                has_found = true;
                break;
            }
        }
        catch (...) {
            continue;
        }
    }

    return has_found
               ? Object(p_encodings->substr(encoding))
               : Object{Nill{}};
}

/**
 * @brief
 *      (iconv "enc-from" "enc-to" "content") -> "converted-out"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_iconv(varlisp::Environment& env, const varlisp::List& args)
{
    Object obj1;
    const string_t * p_enc_from = varlisp::getTypedValue<string_t>(env, args.head, obj1);
    if (!p_enc_from) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(iconv: 1st argument must be encodings string)");
    }
    Object obj2;
    const string_t * p_enc_to = varlisp::getTypedValue<string_t>(env, args.tail[0].head, obj2);
    if (!p_enc_to) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(iconv: 2nd argument must be encodings string)");
    }
    Object obj3;
    const string_t * p_content = varlisp::getTypedValue<string_t>(env, args.tail[0].tail[0].head, obj3);
    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(iconv: 3rd argument must be content string)");
    }

    std::string out;
    sss::iConv ic(p_enc_to->to_string(), p_enc_from->to_string());
    if (!ic.convert(out, p_content->to_string())) {
        COLOG_ERROR("(iconv: error occurs while convert from", *p_enc_from, "to", *p_enc_to);
        return Object{Nill{}};
    }
    return string_t(std::move(out));
}

/**
 * @brief
 *      (ensure-utf8 "content") -> "utf8-content"
 *      (ensure-utf8 "content" "fencodings") -> "utf8-content"
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_ensure_utf8(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "ensure-utf8";
    Object obj2;
    const string_t * p_content = varlisp::getTypedValue<string_t>(env, args.head, obj2);
    if (!p_content) {
        SSS_POSITION_THROW(std::runtime_error,
                          "(", funcName, ": 1st argument must be content string)");
    }

    Object obj1;
    const string_t * p_encodings = 0;
    if (args.length() >= 2) {
        p_encodings = varlisp::getTypedValue<string_t>(env, args.tail[0].head, obj1);
        if (!p_encodings) {
            SSS_POSITION_THROW(std::runtime_error,
                              "(", funcName, ": 2nd argument must be encodings string)");
        }
    }
    std::string encodings;
    std::string to_encoding = "utf8";
    if (p_encodings) {
        encodings = p_encodings->to_string();
    }
    std::string from_encoding;
    std::string content = p_content->to_string();
    if (encodings.empty()) {
        from_encoding = sss::Encoding::detect(content);
    }
    else {
        from_encoding = sss::Encoding::encodings(content, encodings);
    }
    if (!sss::Encoding::isCompatibleWith(from_encoding, to_encoding)) {
        std::string out;
        sss::iConv ic(to_encoding, from_encoding);
        if (ic.convert(out, content)) {
            return string_t(std::move(out));
        }
        else {
            COLOG_ERROR("(", funcName, ":", from_encoding, "to", to_encoding);
            return Object{Nill{}};
        }
    }
    return *p_content;
}

} // namespace varlisp
