// src/detail/consumer.hpp
#pragma once

#include <string_view>
#include <sss/algorithm.hpp>

namespace varlisp {
namespace detail { 

template<typename Func>
inline bool trim_positive_if(std::string_view& sv, Func&& f){
    auto oldSv = sv;
    while (!sv.empty() && f(sv.front())) {
        sv.remove_prefix(1);
    }
    return oldSv.data() != sv.data();
}

template<typename Func>
inline bool trim_one_if(std::string_view& sv, Func&& f) {
    if (!sv.empty() && f(sv.front())) {
        sv.remove_prefix(1);
        return true;
    }
    return false;
}

template<typename Func>
inline bool trim_times_if(std::string_view& sv, int cnt, Func&& f) {
    if (cnt <= 0) {
        return true;
    }

    if (sv.length() < size_t(cnt)) {
        return false;
    }

    auto sub = sv.substr(0, cnt);
    if (sss::is_all(sub, f)) {
        sv.remove_prefix(cnt);
        return true;
    }

    return false;
}

inline bool trim_positive(std::string_view& sv, char c){
    auto oldSv = sv;
    while (!sv.empty() && sv.front() == c) {
        sv.remove_prefix(1);
    }
    return oldSv.data() != sv.data();
}

inline bool trim_one(std::string_view& sv, char c) {
    if (!sv.empty() && sv.front() == c) {
        sv.remove_prefix(1);
        return true;
    }
    return false;
}

inline bool trim_times(std::string_view& sv, int cnt, char c) {
    if (cnt <= 0) {
        return true;
    }

    if (sv.length() < size_t(cnt)) {
        return false;
    }

    auto sub = sv.substr(0, cnt);
    if (sss::is_all(sub, [=](char ch)->bool { return ch == c;})) {
        sv.remove_prefix(cnt);
        return true;
    }

    return false;
}

} // namespace detail
} // namespace varlisp
