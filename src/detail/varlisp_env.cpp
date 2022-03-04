#include "varlisp_env.hpp"

#include <sss/path.hpp>
#include <sss/environ.hpp>

extern char ** environ;

namespace varlisp {
namespace detail {
namespace envmgr {

struct pgr_wrapper_t {
    sss::PenvMgr2 mgr;
    pgr_wrapper_t()
    {
        std::string root = sss::path::dirname(sss::path::getbin());
        mgr.set("root", root);
        // 脚本默认搜索路径
        mgr.set("root_script", sss::path::append_copy(root, "script"));
        // cookie默认搜索路径
        mgr.set("root_cookie", sss::path::append_copy(root, "cookie"));

        // 导入系统环境变量
        char ** p_env = environ;
        while (p_env && p_env[0]) {
            int eq_pos = std::strchr(p_env[0], '=') - p_env[0];
            mgr.set(std::string(p_env[0], eq_pos), std::string(p_env[0] + eq_pos + 1));
            p_env++;
        }
    }
};

sss::PenvMgr2& get_instance()
{
    static pgr_wrapper_t mgr;
    return mgr.mgr;
}

// NOTE
// expand就是只做$替换；至于路径补全，用fnamemodify就好。
std::string expand(const std::string& path)
{
    if (path.find('$') != std::string::npos) {
        return envmgr::get_instance().get_expr(path);
    }
    else {
        return path;
    }
}

} // namespace envmgr
} // namespace detail
} // namespace varlisp
