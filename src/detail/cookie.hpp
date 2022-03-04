#pragma once

#include <string>

namespace varlisp {
namespace detail {
struct CookieMgr_t
{
    // 逻辑：
    // 通过domain，在cookie文件夹下，搜索匹配的文件；
    // 然后读取内部的值——根据path不同，而不同的ini文件。
    // 根据路径匹配情况，选取合适的cookie，并返回。

    static std::string getCookie(const std::string& domain,
                                 const std::string& path);

    static bool        setCookie(const std::string& domain,
                                 const std::string& path,
                                 const std::string& cookie);

    static bool get_cookie_enable_status();
    static void set_cookie_enable_status(bool status);
};
} // namespace detail
} // namespace varlisp
