#include "builtin_helpdoc_mgr.hpp"

#include <cctype>

#include <fstream>
#include <iostream>

#include <sss/spliter.hpp>
#include <sss/string_view.hpp>

namespace varlisp {
builtin_helpdoc_mgr::builtin_helpdoc_mgr(const std::string& helpdoc_fname)
    : m_helpdoc_fname(helpdoc_fname)
{
    std::ifstream ifs(m_helpdoc_fname, std::ios_base::in | std::ios_base::binary);
    std::string line;
    while (std::getline(ifs, line)) {
        sss::string_view s{line};
        sss::ViewSpliter<char> vs(s, '\t');
        sss::string_view item;
        while (vs.fetch(item)) {
            this->m_doc_map[item.to_string()] = varlisp::string_t{s.substr(item.size() + 1).to_string()};
        }
    }
}

varlisp::string_t builtin_helpdoc_mgr::get(const std::string& name) const
{
    auto it = m_doc_map.find(name);
    if (it != m_doc_map.end()) {
        return it->second;
    }
    else {
        return varlisp::string_t{std::move("cannot find helpdoc for " + name)};
    }
}

} // namespace varlisp
