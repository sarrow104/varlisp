#ifndef __UTIL_HPP_1457603536__
#define __UTIL_HPP_1457603536__

#include <string>

namespace varlisp {
    namespace util {
        inline std::string escape(const std::string& s) {
            // TODO
            return '"' + s + '"';
        }
    } // namespace util
} // namespace varlisp


#endif /* __UTIL_HPP_1457603536__ */
