#include "file.hpp"

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include <map>
#include <stdexcept>

#include <sss/util/PostionThrow.hpp>
#include <sss/path.hpp>

namespace varlisp {
namespace detail {
namespace file {

std::string get_fname_from_fd(int fd)
{
    const size_t buf_size = 256;
    char s[buf_size];
    std::snprintf(s, buf_size - 1, "/proc/%d/fd/%d", getpid(), fd);
    std::string name;
    name.resize(buf_size);
    while (true) {
        int ec = readlink(s, const_cast<char*>(&name[0]), name.size() - 1);
        if (ec > 0) {
            name.resize(ec);
            break;
        }
        else if (ec == -1) {
            switch (errno) {
                case ENAMETOOLONG:
                    name.resize(name.size() * 2);
                    continue;

                default:
                    SSS_POSITION_THROW(std::runtime_error,
                                       std::strerror(errno));
                    break;
            }
        }
        else {
            SSS_POSITION_THROW(std::runtime_error,
                               "unkown readlink return value:", ec,
                               "; errno = ", errno, std::strerror(errno));
        }
    }
    return name;
}

static void on_exit();

typedef std::map<int, std::string> registered_fd_path_map_type;

registered_fd_path_map_type& get_fd_path_map()
{
    static registered_fd_path_map_type fd_path_map;
    static bool reg_on_exit = []() -> bool {
        return std::atexit(on_exit) != 0;
    }();
    (void) reg_on_exit;
    return fd_path_map;
}

bool register_fd(int fd)
{
    std::string path;
    try {
        path = get_fname_from_fd(fd);
        return get_fd_path_map().insert(std::make_pair(fd, path)).second;
    }
    catch (...) {
        return false;
    }
}

bool unregister_fd(int fd)
{
    auto& m = get_fd_path_map();
    auto it = m.find(fd);
    if (it == m.end()) {
        return false;
    } else {
        auto path = it->second;
        m.erase(it);
        sss::path::remove(path);

        return true;
    }
}

static void on_exit()
{
    auto& m = get_fd_path_map();
    auto it = m.begin();
    while (it != m.end()) {
        auto path = it->second;
        it = m.erase(it);
        sss::path::remove(path);
    }
}

} // namespace file
} // namespace detail
} // namespace varlisp
