// src/detail/gfw_base.hpp

#pragma once

#include <string>

#include <memory>

namespace re2 {

class RE2;

} // namespace re2

namespace varlisp {
namespace detail {

bool gfw_need_proxy_with(const re2::RE2& reg, const std::string &url);

// : std::enable_shared_from_this<gfw_base>
struct gfw_base : std::enable_shared_from_this<gfw_base> {
    virtual ~gfw_base() = default;

    virtual bool need_proxy(const std::string& url) const = 0;
    virtual int get_port() const = 0;
    virtual std::string get_host() const = 0;

    static std::shared_ptr<gfw_base> make_mgr(int type, std::string omegaPath);
};

} // namespace detail
} // namespace varlisp
