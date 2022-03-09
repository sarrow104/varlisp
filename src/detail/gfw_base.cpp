// src/detail/gfw_base.cpp
#include "gfw_base.hpp"
#include "gfw_omegaoption.hpp"

#include "consumer.hpp"

#include <memory>

namespace varlisp::detail {

std::shared_ptr<gfw_base> gfw_base::make_mgr(int type, std::string omegaPath)
{
    (void)type;
    switch (type) {
    case 0:
        return std::make_shared<gfw_omegaoption>(omegaPath);
    default:
        throw std::runtime_error("not implement");
    }
    return nullptr;
}

bool gfw_need_proxy_with(const re2::RE2& reg, const std::string &url)
{
    // NOTE
    // 1. 先fullUrl比较一次；如果命中，就返回true；
    // 2. 如果没有命中，则再用去掉协议头的domain+path部分，再匹配一次
    // 只要其中一次命中，就返回true；否则返回false；
    std::string_view sv = url;

    auto res1 = RE2::PartialMatch(re2::StringPiece{sv.data(), sv.size()}, reg);
    if (res1) {
        return true;
    }

    if (trim_positive_if(sv, [](char c)->bool { return std::isalpha(c) != 0; }) &&
        trim_one(sv, ':') &&
        trim_times(sv, 2, '/'))
    {

        auto res2 = RE2::PartialMatch(re2::StringPiece{sv.data(), sv.size()}, reg);
        if (res2) {
            return true;
        }
    }

    return false;
}

} // namespace varlisp::detail

