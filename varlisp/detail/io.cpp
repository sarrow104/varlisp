// varlisp/detail/io.cpp
#include "io.hpp"

#include <linenoise.hpp>

namespace varlisp {

namespace detail {

std::string readline_stdin()
{
    return linenoise::Readline("");
}

} // namespace detail

} // namespace varlisp
