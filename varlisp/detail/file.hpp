#pragma once

#include <string>

namespace varlisp {
namespace detail {
namespace file {

std::string get_fname_from_fd(int fd);

bool register_fd(int fd);
bool unregister_fd(int fd);

} // namespace file
} // namespace detail
} // namespace varlisp
