#pragma once

#include <string>
#include <functional>

namespace varlisp {
namespace detail {
namespace file {

std::string get_fname_from_fd(int fd);

bool register_fd(int fd);
bool unregister_fd(int fd);
void list_opened_fd(std::function<void(int fd, const std::string& path)> && func);

} // namespace file
} // namespace detail
} // namespace varlisp
