#include "file.hpp"

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

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

} // namespace file
} // namespace detail
} // namespace varlisp
