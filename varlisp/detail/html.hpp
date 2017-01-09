#pragma once

#include <iostream>
#include <map>

#include <ss1x/asio/headers.hpp>

#include "../object.hpp"

namespace varlisp {
namespace detail {
namespace html {

enum file_status_t {
    fs_NONE,    // 还未下载
    fs_EXIST,   // 已经存在
    fs_ERROR,   // 遇到错误，无法下载
    fs_PARTIAL, // 因为某种原样，部分下载
    fs_DONE     // 下载完成
};

struct local_info_t {
    local_info_t() {};
    local_info_t(const std::string& path, uint64_t size, file_status_t fs)
        : path(path), fsize(size), status(fs){};
    std::string   path;
    uint64_t      fsize = 0;
    file_status_t status = fs_NONE;

    bool is_ok() const {
        return this->status == fs_DONE || this->status == fs_EXIST;
    }
    void print(std::ostream& o) const {
        const char * name_table[] = {
            "NONE",
            "EXIST",
            "ERROR",
            "PARTIAL",
            "DONE",
        };
        o << "{";
        o << "\"path\": " << sss::raw_string(path) << ",";
        o << "\"fsize\": " << fsize << ",";
        o << "\"status\": " << sss::raw_string(name_table[status]);
        o << "}";
    }
};

inline std::ostream& operator << (std::ostream& o, const local_info_t& i)
{
    i.print(o);
    return o;
}

using resource_manager_t = std::map<std::string, local_info_t>;

void gumbo_rewrite_impl(int fd, const gumboNode& g,
                        const std::string& output_dir,
                        resource_manager_t& rs_mgr,
                        const ss1x::http::Headers& request_header,
                        const std::string& proxy_domain = "",
                        int proxy_port = 0);

void         set_gqnode_indent(const std::string& ind);
std::string& get_gqnode_indent();

} // namespace html
} // namespace detail
} // namespace varlisp
