// src/detail/config.cpp
#include "config.hpp"
#include "env_get_value.hpp"

#include <string>

#include <sss/path.hpp>

namespace varlisp {
namespace detail {

namespace config
{

std::string get_omegaGfwRulePath()
{
    static const std::string omegaOptionPathDefault =
        sss::path::append_copy(sss::path::dirname(sss::path::getbin()), "OmegaOptions.bak");

    return get_value_with_default("path-to-omegaoptions", omegaOptionPathDefault);
}

} // namespace config

} // namespace detail
} // namespace varlisp
