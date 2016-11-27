#pragma once

#include <string>
#include <map>

#include "String.hpp"

namespace varlisp {
class builtin_helpdoc_mgr
{
public:
    explicit builtin_helpdoc_mgr(const std::string& helpdoc_fname);
    ~builtin_helpdoc_mgr() = default;
    string_t get(const std::string& name) const;
 
private:
    std::string m_helpdoc_fname;
    std::map<std::string, varlisp::string_t> m_doc_map;
};
} // namespace varlisp
