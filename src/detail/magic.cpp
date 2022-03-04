#include "magic.hpp"

using namespace std;

namespace FileTyping
{

Magic::Magic() :
    m_handle(::magic_open(MAGIC_MIME_TYPE | MAGIC_SYMLINK))
{
    ::magic_load(m_handle, NULL);
}

Magic::~Magic()
{
    ::magic_close(m_handle);
}

std::string Magic::file(const string& filepath)
{
    return ::magic_file(m_handle, filepath.c_str());
}

std::string Magic::buffer(const vector<unsigned char>& raw)
{
    return ::magic_buffer(m_handle, raw.data(), raw.size());
}

std::string Magic::buffer(const char* data, size_t size)
{
    return ::magic_buffer(m_handle, data, size);
}


} // FileTyping
