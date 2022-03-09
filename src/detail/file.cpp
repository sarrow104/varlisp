#include "file.hpp"

#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>

#include <functional>
#include <map>
#include <stdexcept>

#include <sss/macro/defer.hpp>
#include <sss/path.hpp>
#include <sss/path/glob_path.hpp>
#include <sss/util/PostionThrow.hpp>

#if defined (__APPLE__)
#   include <fcntl.h>
#   include <libproc.h>
#   include <sys/proc_info.h>
#   include <sys/syslimits.h>
#endif

namespace varlisp::detail::file {

std::string get_fname_from_fd(int fd)
{
    std::string name;
#if defined (__linux__)
    const size_t buf_size = 256;
    char s[buf_size];
    std::snprintf(s, buf_size - 1, "/proc/%d/fd/%d", getpid(), fd);
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
#elif defined (__APPLE__)
	// https://stackoverflow.com/questions/58081928/c-print-file-path-from-file/58082106#58082106
	char filePath[PATH_MAX] = {0};
	if (fcntl(fd, F_GETPATH, filePath) != -1)
	{
		name.assign(filePath);
	}
#endif
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
    }
    auto path = it->second;
    m.erase(it);
    sss::path::remove(path);

    return true;
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

void list_opened_fd(std::function<void(int fd, const std::string& path)> && func) {
#if defined (__APPLE__)
    int pid = getpid();
    int bufferSize = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, 0, 0);
    auto *procFdInfo = (struct proc_fdinfo*)malloc(bufferSize);
    SSS_DEFER(free(procFdInfo));

    bufferSize = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, procFdInfo,
                              (size_t)bufferSize);

    int numberOfProcFds = bufferSize / PROC_PIDLISTFD_SIZE;

    for (int i = 0; i < numberOfProcFds; ++i) {
        switch (procFdInfo[i].proc_fdtype) {
        case PROX_FDTYPE_VNODE:
            {
                struct vnode_fdinfowithpath vnodeInfo{};
                int nb = proc_pidfdinfo(pid, procFdInfo[i].proc_fd, PROC_PIDFDVNODEPATHINFO,
                                        &vnodeInfo, PROC_PIDFDVNODEPATHINFO_SIZE);

                (void)nb;
                func(procFdInfo[i].proc_fd, std::string(vnodeInfo.pvip.vip_path));
            }
            break;

        default:
            // nothing todo
            break;
        }
    }
#else
    const size_t buf_size = 256;
    char dir[buf_size];
    std::snprintf(dir, buf_size - 1, "/proc/%d/fd/", getpid());
    sss::path::file_descriptor fd;
    sss::path::glob_path gp(dir, fd);
    varlisp::List ret;
    auto back_it = varlisp::detail::list_back_inserter<varlisp::Object>(ret);
    while (gp.fetch()) {
        if (!fd.is_normal_file()) {
            continue;
        }
        if (!std::isdigit(fd.get_name()[0])) {
            continue;
        }
        int id = sss::string_cast<int>(fd.get_name());
        func(id, detail::file::get_fname_from_fd(id));
    }
#endif
}

} // namespace varlisp::detail::file
