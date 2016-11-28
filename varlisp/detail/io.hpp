#pragma once

#include <string>

#include <unistd.h>

#include <sss/string_view.hpp>
#include <sss/util/utf8.hpp>

namespace varlisp {
namespace detail {
inline std::string readline(int fd)
{
    std::string line;
    char ch = '\0';
    int ec = 0;
    while ((ec = ::read(fd, &ch, 1)) != -1) {
        if (ec == 0) {
            return line;
        }
        line += ch;
        if (ch == '\n') {
            break;
        }
    }
    return line;
}
inline int readchar(int fd)
{
    char buf[6];
    int ec = ::read(fd, buf, 1);
    if (ec == -1) {
        return -1;
    }
    int len = sss::util::utf8::next_length(buf, buf + 1);
    if (len == 1) {
        return int(buf[0]);
    }
    else {
        ec = ::read(fd, buf + 1, len - 1);
        if (ec != len - 1) {
            return -1;
        }
        auto ch = sss::util::utf8::peek(buf, buf + len);
        if (ch.second != len) {
            return -1;
        }
        return int(ch.first);
    }
}

inline int writechar(int fd, int ch)
{
    char buf[6];
    if (ch && ch != -1) {
        int len = sss::util::utf8::get_ucs_code_length(ch);
        sss::util::utf8::dumpout2utf8(&ch, &ch + 1, buf);
        ::write(fd, buf, len);
        return len;
    }
    return 0;
}

inline int writestring(int fd, sss::string_view sv)
{
    return ::write(fd, sv.data(), sv.size());
}
} // namespace detail
} // namespace varlisp

