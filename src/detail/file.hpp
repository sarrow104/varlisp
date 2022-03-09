#pragma once

#include <functional>
#include <string>

namespace varlisp::detail::file {

std::string get_fname_from_fd(int fd);

bool register_fd(int fd);
bool unregister_fd(int fd);
void list_opened_fd(std::function<void(int fd, const std::string& path)> && func);

} // namespace varlisp::detail::file
