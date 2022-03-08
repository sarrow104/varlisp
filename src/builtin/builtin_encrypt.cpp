#include <array>

#include <zlib.h>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>

#include <sss/enc/base64.hpp>

#include "../builtin_helper.hpp"
#include "../object.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../environment.hpp"

namespace varlisp {
REGIST_BUILTIN("en-base64", 1, 1, eval_en_base64,
               "(en-base64 \"string\") -> \"enc\"");

Object eval_en_base64(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "en-base64";
    std::array<Object, 1> objs;
    const auto* p_str = requireTypedValue<varlisp::string_t>(
        env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    sss::enc::Base64 b;
    return varlisp::string_t(b.encode(*p_str->gen_shared()));
}

REGIST_BUILTIN("de-base64", 1, 1, eval_de_base64,
               "(de-base64 \"string\") -> \"decode\"");

Object eval_de_base64(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "de-base64";
    std::array<Object, 1> objs;
    const auto* p_str = requireTypedValue<varlisp::string_t>(
        env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    sss::enc::Base64 b;
    return varlisp::string_t(b.decode(*p_str->gen_shared()));
}

REGIST_BUILTIN("en-gzip", 1, 1, eval_en_gzip,
               "(en-gzip \"string\") -> \"enc\"");

// https://en.wikipedia.org/wiki/Gzip
Object eval_en_gzip(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "en-gzip";
    std::array<Object, 1> objs;
    const auto* p_str = requireTypedValue<varlisp::string_t>(
        env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(boost::iostreams::gzip_compressor());
    std::istringstream iss(p_str->to_string());
    std::ostringstream oss;
    in.push(iss);
    boost::iostreams::copy(in, oss);

    return varlisp::string_t(oss.str());
}

// #include <fstream>
// #include <iostream>
// #include <boost/iostreams/filtering_streambuf.hpp>
// #include <boost/iostreams/copy.hpp>
// #include <boost/iostreams/filter/gzip.hpp>
//
// int main()
// {
//     using namespace std;
//
//     ifstream file("hello.gz", ios_base::in | ios_base::binary);
//     filtering_streambuf<input> in;
//     in.push(gzip_decompressor());
//     in.push(file);
//     boost::iostreams::copy(in, cout);
// }

REGIST_BUILTIN("de-gzip", 1, 1, eval_de_gzip,
               "(de-gzip \"string\") -> \"dec\"");

Object eval_de_gzip(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "de-gzip";
    std::array<Object, 1> objs;
    const auto* p_str = requireTypedValue<varlisp::string_t>(
        env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
    in.push(boost::iostreams::gzip_decompressor());
    std::istringstream iss(p_str->to_string());
    std::ostringstream oss;
    in.push(iss);
    boost::iostreams::copy(in, oss);

    return varlisp::string_t(oss.str());
}

namespace detail {
inline void calcWindowBits(int& windowBits, sss::string_view method_name,
                           const char* funcName)
{
    if (method_name == sss::string_view("gzip")) {
        windowBits = 16 + MAX_WBITS;
    }
    else if (method_name == sss::string_view("zlib")) {
        windowBits = MAX_WBITS;
    }
    else if (method_name == sss::string_view("deflate")) {
        windowBits = -MAX_WBITS;
    }
    else {
        SSS_POSITION_THROW(std::runtime_error, "wrong ", funcName, " method ",
                           method_name);
    }
}

inline void calcLevel(int& level, int in_level, const char* funcName)
{
    if (in_level < 0 || in_level > 9) {
        SSS_POSITION_THROW(std::runtime_error, "wrong ", funcName,
                           " level; out of range [0, 9], ", in_level);
    }
    level = in_level;
}

}  // namespace detail

REGIST_BUILTIN("deflate", 2, 3, eval_deflate,
               "; deflate 算法压缩\n"
               "; 参考： $Node.js/node-deflate/src/deflate.cc\n"
               "; 参数0： 目标待压缩串\n"
               "; 参数1： 压缩级别；整数；0-9；或者算法 gzip,zlib,deflate\n"
               "; 参数2： 可选，压缩级别，整数0-9\n"
               "(deflate \"string\") -> \"dec\"");

Object eval_deflate(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "deflate";
    std::array<Object, 3> objs;
    const auto* p_str = requireTypedValue<varlisp::string_t>(
        env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    int level = Z_DEFAULT_COMPRESSION;
    int windowBits = 16 + MAX_WBITS;  // gzip
    int memLevel = 8;
    int strategy = Z_DEFAULT_STRATEGY;

    if (args.size() >= 3) {
        const auto* p_method_name = requireTypedValue<varlisp::string_t>(
            env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

        detail::calcWindowBits(windowBits, p_method_name->to_string_view(),
                               funcName);

        detail::calcLevel(level,
                          *requireTypedValue<int64_t>(env, args.nth(2), objs[2],
                                                      funcName, 2, DEBUG_INFO),
                          funcName);
    }
    else {
        const auto& value = varlisp::getAtomicValue(env, args.nth(1), objs[1]);
        if (const auto* p_method_name = boost::get<string_t>(&value)) {
            detail::calcWindowBits(windowBits, *p_method_name, funcName);
        }
        else if (const auto* p_level = boost::get<int64_t>(&value)) {
            detail::calcLevel(level, *p_level, funcName);
        }
        else {
            SSS_POSITION_THROW(std::runtime_error, "wrong type of 2nd args of ",
                               funcName, " must be string or int; but ", value);
        }
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    int r =
        deflateInit2(&strm, level, Z_DEFLATED, windowBits, memLevel, strategy);

    if (r < 0) {
        SSS_POSITION_THROW(std::runtime_error, "zlib error ",
                           std::strerror(errno));
    }

    // deflate
    strm.avail_in = p_str->size();
    strm.next_in =
        reinterpret_cast<unsigned char*>(const_cast<char*>(p_str->data()));

    uLong bound = deflateBound(&strm, strm.avail_in);

    std::vector<char> out;
    out.resize(bound);
    // char* out = (char*)malloc(bound);

    strm.avail_out = bound;
    strm.next_out = reinterpret_cast<Bytef*>(out.data());

    r = deflate(&strm, Z_FINISH);

    int out_length = bound - strm.avail_out;

    deflateEnd(&strm);

    if (r < 0) {
        SSS_POSITION_THROW(std::runtime_error, "zlib error ",
                           std::strerror(errno));
    }

    // output
    string_t ret{std::string(out.data(), out_length)};

    return ret;
}

REGIST_BUILTIN("inflate", 1, 2, eval_inflate,
               "; deflate 算法解压\n"
               "; 第一个参数：输入的buffer\n"
               "; 第二个参数：gzip | zlib | deflate\n"
               "(inflate \"string\") -> \"dec\"");

Object eval_inflate(varlisp::Environment& env, const varlisp::List& args)
{
    int windowBits = 16 + MAX_WBITS;  // gzip

    const char* funcName = "inflate";
    std::array<Object, 2> objs;
    const auto* p_str = requireTypedValue<varlisp::string_t>(
        env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    enum deflate_method_t { dm_gzip = 0, dm_zlib = 1, dm_deflate = 2 };

    if (args.size() >= 2) {
        const auto* p_method_name = requireTypedValue<varlisp::string_t>(
            env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

        detail::calcWindowBits(windowBits, p_method_name->to_string_view(), funcName);
    }

    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    int r = inflateInit2(&strm, windowBits);

    if (r < 0) {
        SSS_POSITION_THROW(std::runtime_error, "zlib error ",
                           std::strerror(errno));
    }

    if (p_str->empty()) {
        return Nill{};
    }

    // deflate
    strm.avail_in = p_str->size();
    strm.next_in =
        reinterpret_cast<unsigned char*>(const_cast<char*>(p_str->data()));

    // urgh, we don't know the buffer size (Q: is it in the gzip header?)
    uLong bound = 128 * 1024;  // 128K =  131072 byte

    std::vector<char> out;
    out.resize(bound);

    strm.avail_out = bound;
    strm.next_out = reinterpret_cast<Bytef*>(out.data());

    r = inflate(&strm, Z_FINISH);

    while (r == Z_BUF_ERROR) {
        bound = bound * 2;
        size_t len = (char*)strm.next_out - out.data();

        out.resize(bound);

        strm.avail_out = bound - len;
        strm.next_out = reinterpret_cast<Bytef*>(out.data() + len);

        r = inflate(&strm, Z_FINISH);
    }

    if (r < 0) {
        SSS_POSITION_THROW(std::runtime_error, "zlib error ",
                           std::strerror(errno));
    }

    // NOTE 此时，strm.avail_out 表示还有多少空闲空间没有使用——
    int out_length = bound - strm.avail_out;

    inflateEnd(&strm);

    // output
    string_t ret{std::string(out.data(), out_length)};

    return ret;
}

}  // namespace varlisp
