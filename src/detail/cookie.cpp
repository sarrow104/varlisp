#include "cookie.hpp"
#include "varlisp_env.hpp"

#include <sss/utlstring.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/dosini/dosini.hpp>
#include <sss/path.hpp>
#include <sss/path/glob_path.hpp>
#include <sss/path/name_filter.hpp>

namespace varlisp {
namespace detail {
    // 逻辑：
    // 通过domain，在cookie文件夹下，搜索匹配的文件；
    // 然后读取内部的值——根据path不同，而不同的ini文件。
    // 根据路径匹配情况，选取合适的cookie，并返回。

const std::string& get_cookie_path()
{
    static std::string cookie_path = varlisp::detail::envmgr::expand("$root_cookie");
    return cookie_path;
}

std::string findCfgPath(const std::string& cookie_path, const std::string& domain)
{
    sss::path::file_descriptor fd;
    sss::path::name_filter_t f("*.ini");
    sss::path::glob_path gp(cookie_path, fd, &f);
    std::string possible_cookie_path;
    while (gp.fetch()) {
        if (fd.is_normal_file()) {
            COLOG_DEBUG(fd.get_path());
            if (fd.get_name() && sss::is_end_with(domain, sss::path::no_suffix(fd.get_name()))) {
                if (fd.get_path().size() > possible_cookie_path.size()) {
                    possible_cookie_path = fd.get_path();
                }
            }
        }
    }
    return possible_cookie_path;
}

std::string CookieMgr_t::getCookie(const std::string& domain,
                                   const std::string& path)
{
    std::string cookie_cfg_path = findCfgPath(get_cookie_path(), domain);

    if (cookie_cfg_path.empty()) {
        return "";
    }
    COLOG_ERROR(SSS_VALUE_MSG(cookie_cfg_path));
    sss::dosini ini(cookie_cfg_path);
    for (auto block_it = ini.begin(); block_it != ini.end(); ++block_it) {
        COLOG_ERROR("[", block_it->first, "]");
        if (sss::is_begin_with(path, block_it->first)) {
            auto kv_it = block_it->second.find("value");
            if (kv_it == block_it->second.end()) {
                return "";
            }
            return kv_it->second.get();
        }
    }
    return "";
}

static bool cookie_enable_status = true;

bool CookieMgr_t::get_cookie_enable_status()
{
    return cookie_enable_status;
}

void CookieMgr_t::set_cookie_enable_status(bool status)
{
    cookie_enable_status = status;
}

bool        CookieMgr_t::setCookie(const std::string& domain,
                                   const std::string& path,
                                   const std::string& cookie)
{
    std::string cookie_cfg_path = findCfgPath(get_cookie_path(), domain);

    if (cookie_cfg_path.empty()) {
        cookie_cfg_path = get_cookie_path();
        sss::path::append(cookie_cfg_path, domain + ".ini");
    }
    COLOG_ERROR(SSS_VALUE_MSG(cookie_cfg_path));
    sss::dosini ini(cookie_cfg_path);
    ini.set(path, "value", cookie);
    return ini.write(cookie_cfg_path);
}

} // namespace detail
} // namespace varlisp
