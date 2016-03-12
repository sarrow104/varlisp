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
        typedef std::map<std::string, Object>::const_iterator const_iterator;
        typedef std::map<std::string, Object>::iterator iterator;

        const_iterator find(const std::string& name) const;
        iterator find(const std::string& name);
        // Object& operator[](const std::string& name);

        using BaseT::begin;
        using BaseT::end;
        using BaseT::cbegin;
        using BaseT::cend;
        using BaseT::operator[];

        explicit Environment(Environment * parent = 0);
        Environment * m_parent;
    };
} // namespace varlisp


#endif /* __EVIRONMENT_HPP_1457164527__ */
