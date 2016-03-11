#ifndef __EVIRONMENT_HPP_1457164527__
#define __EVIRONMENT_HPP_1457164527__

#include <map>
#include <string>

#include <memory>

#include "object.hpp"

namespace varlisp {
    struct Environment : public std::map<std::string, Object>
    {
        typedef std::map<std::string, Object> BaseT;
        using BaseT::find;
        using BaseT::begin;
        using BaseT::end;
        using BaseT::cbegin;
        using BaseT::cend;
        Environment * m_parent;
    };
} // namespace varlisp


#endif /* __EVIRONMENT_HPP_1457164527__ */
