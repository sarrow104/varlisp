// clone from https://github.com/ThibsG/file_typing
#ifndef MAGIC_HPP
#define MAGIC_HPP

#include <string>
#include <vector>

extern "C" {
#include <magic.h>
}

namespace FileTyping
{

class Magic
{
public:
    explicit Magic();
    Magic(const Magic&) = delete;
    Magic(Magic&&) = delete;
    ~Magic();

    std::string file(const std::string& filepath);
    std::string buffer(const std::vector<unsigned char>& raw);
    std::string buffer(const char* data, size_t size);

private:
    magic_t m_handle;
};

} // FileTyping

#endif
