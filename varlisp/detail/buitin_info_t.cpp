#include "buitin_info_t.hpp"

#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

namespace varlisp {
namespace detail {
builtin_info_vet_t& get_builtin_infos()
{
    static builtin_info_vet_t m_builtin_infos;
    return m_builtin_infos;
}

void builtin_info_t::params_size_check(int arg_len) const
{
    if (this->min > 0 && arg_len < this->min) {
        SSS_POSITION_THROW(std::runtime_error, this->name, " need at least ",
                           this->min, " parameters. but ", arg_len,
                           " argument(s) provided.");
    }
    if (this->max > 0 && arg_len > this->max) {
        SSS_POSITION_THROW(std::runtime_error, this->name, " need at most ",
                           this->max, " parameters. but ", arg_len,
                           " arguments provided.");
    }
}

} // namespace detail

} // namespace varlisp
