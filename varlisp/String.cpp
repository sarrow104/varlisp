#include "String.hpp"
#include <iostream>

namespace varlisp {

String& String::operator=(const std::string& s)
{
    this->clear();
    if (!s.empty()) {
        this->m_refer = std::make_shared<std::string>(s);
        sss::string_view::operator=(*m_refer);
    }
    return *this;
}
String& String::operator=(std::string&& s)
{
    this->clear();
    if (!s.empty()) {
        this->m_refer = std::make_shared<std::string>(std::move(s));
        sss::string_view::operator=(*m_refer);
    }
    return *this;
}

void String::print(std::ostream&o) const
{
    o << static_cast<const sss::string_view&>(*this);
}
} // namespace varlisp
