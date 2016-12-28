#pragma once

#include <string>

#include <unistd.h>

#include <sss/string_view.hpp>
#include <sss/util/utf8.hpp>

namespace varlisp {
namespace detail {

inline std::string readline(int64_t fd)
{
    std::string line;
    char ch = '\0';
    int64_t ec = 0;
    while ((ec = ::read(fd, &ch, 1)) != -1) {
        if (ec == 0) {
            return line;
        }
        if (ch == '\n') {
            break;
        }
        line += ch;
    }
    return line;
}

inline int64_t readchar(int64_t fd)
{
    char buf[6];
    int64_t ec = ::read(fd, buf, 1);
    if (ec == -1) {
        return -1;
    }
    int64_t len = sss::util::utf8::next_length(buf, buf + 1);
    if (len == 1) {
        return int64_t(buf[0]);
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
        return int64_t(ch.first);
    }
}

inline int64_t readbyte(int64_t fd)
{
    char buf[1];
    int64_t ec = ::read(fd, buf, 1);
    if (ec == -1) {
        return -1;
    }
    return int64_t(uint8_t(buf[0]));
}

inline int64_t writechar(int64_t fd, int64_t ch)
{
    char buf[6];
    if (ch && ch != -1) {
        int64_t len = sss::util::utf8::get_ucs_code_length(ch);
        sss::util::utf8::dumpout2utf8(&ch, &ch + 1, buf);
        // 返回值包括-1状态！
        return ::write(fd, buf, len);;
    }
    return 0;
}

inline int64_t writebyte(int64_t fd, int64_t ch)
{
    char buf[1];
    if (ch != -1) {
        buf[0] = uint8_t(fd & 0xFFu);
        return ::write(fd, buf, sizeof(buf));
    }
    return 0;
}

inline int64_t writestring(int64_t fd, sss::string_view sv)
{
    return ::write(fd, sv.data(), sv.size());
}
} // namespace detail
} // namespace varlisp

