#include "varlisp_env.hpp"

namespace varlisp {
namespace detail {

sss::PenvMgr2& get_envmgr()
{
    static sss::PenvMgr2 mgr;
    static bool init_mgr = [](sss::PenvMgr2& mgr)->bool {
        std::string root = sss::path::dirname(sss::path::getbin());
        mgr.set("root", root);
        mgr.set("root_script", sss::path::append_copy(root, "script"));
        return true;
    }(mgr);
    (void)init_mgr;
    return mgr;
}
} // namespace detail
} // namespace varlisp
