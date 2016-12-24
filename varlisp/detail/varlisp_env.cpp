#include "varlisp_env.hpp"

#include <sss/path.hpp>

namespace varlisp {
namespace detail {
namespace envmgr {

sss::PenvMgr2& get_instance()
{
    static sss::PenvMgr2 mgr;
    static bool init_mgr = [](sss::PenvMgr2& mgr)->bool {
        std::string root = sss::path::dirname(sss::path::getbin());
        mgr.set("root", root);
        // 脚本默认搜索路径
        mgr.set("root_script", sss::path::append_copy(root, "script"));
        // cookie默认搜索路径
        mgr.set("root_cookie", sss::path::append_copy(root, "cookie"));
        return true;
    }(mgr);
    (void)init_mgr;
    return mgr;
}

// NOTE TODO
// 这里应该分开；expand就是只做$替换；至于路径补全，用fnamemodify就好。
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
