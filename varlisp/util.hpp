#ifndef __UTIL_HPP_1457603536__
#define __UTIL_HPP_1457603536__

#include <string>

#include <sss/raw_print.hpp>
#include <sstream>

namespace varlisp {
    namespace util {
        inline std::string escape(const std::string& s) {
            std::ostringstream oss;
            oss << sss::raw_string(s);
            return oss.str();
        }
    } // namespace util
} // namespace varlisp


#endif /* __UTIL_HPP_1457603536__ */
