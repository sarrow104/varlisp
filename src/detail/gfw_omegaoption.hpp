// src/detail/gfw_omegaoption.hpp
#pragma once

#include "gfw_base.hpp"

#include <re2/re2.h>

#include <memory>

namespace varlisp::detail {

struct gfw_omegaoption : public gfw_base {
    explicit gfw_omegaoption(std::string omegaPath);
    ~gfw_omegaoption() override;

    std::string host;
    int port;
    std::shared_ptr<RE2> urlWholeReg;

    bool need_proxy(const std::string &url) const override;

    std::string get_host() const override;

    int get_port() const override;
};

} // namespace varlisp::detail
