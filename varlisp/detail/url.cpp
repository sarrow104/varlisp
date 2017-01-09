#include "url.hpp"

#include <ss1x/asio/utility.hpp>

#include <sss/colorlog.hpp>

namespace varlisp {
namespace detail {
namespace url {

bool full_of(std::string& target, const std::string& mapping)
{
    auto targets = ss1x::util::url::split_port_auto(target);
    auto mappings = ss1x::util::url::split_port_auto(mapping);
    COLOG_DEBUG(targets, mappings);
    bool is_modified = false;
    if (std::get<0>(targets).empty() && !std::get<0>(mappings).empty()) {
        std::get<0>(targets) = std::get<0>(mappings);
        is_modified = true;
    }
    if (std::get<1>(targets).empty() && !std::get<1>(mappings).empty()) {
        std::get<1>(targets) = std::get<1>(mappings);
        is_modified = true;
    }
    if (std::get<2>(targets) != std::get<2>(mappings) && std::get<2>(mappings)) {
        std::get<2>(targets) = std::get<2>(mappings);
        is_modified = true;
    }
    if (std::get<3>(targets).empty()) {
        SSS_POSITION_THROW(std::runtime_error,
                           "(", target, ": empty path)");
    }
    if (std::get<3>(targets).front() != '/' && !std::get<1>(mappings).empty()) {
        auto refer = std::get<3>(mappings);
        auto q_pos = refer.find('?');
        if (q_pos != std::string::npos) {
            refer.resize(q_pos);
        }
        if (refer.back() != '/') {
            refer = sss::path::dirname(refer);
        }
        std::get<3>(targets) = sss::path::append_copy(
            refer, std::get<3>(targets));
        is_modified = true;
    }
    if (is_modified) {
        target = ss1x::util::url::join(targets);
    }
    return is_modified;
}
} // namespace url
} // namespace detail
} // namespace varlisp
