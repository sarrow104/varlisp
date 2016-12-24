#include <sss/penvmgr2.hpp>

#include "../object.hpp"

namespace varlisp {
namespace detail {
namespace envmgr {
sss::PenvMgr2& get_instance();
std::string expand(const std::string& path);

} // namespace envmgr
} // namespace detail
} // namespace varlisp
