#include "String.hpp"

#include <iostream>

#include <sss/colorlog.hpp>

namespace varlisp {

std::shared_ptr<std::string> String::gen_shared() const
{
    if (this->m_refer && this->data() == this->m_refer->data() && this->size() == this->m_refer->size()) {
        return this->m_refer;
    }
    return std::make_shared<std::string>(this->to_string());
}

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

// 判断是否以end结尾——即，
const char * String::safe_c_str() const
{
    if (this->m_refer &&
        this->m_refer->c_str() + this->m_refer->size() ==
            this->data() + this->size())
    {
        return this->data();
    }
    return nullptr;
}

void String::print(std::ostream&o) const
{
    o << static_cast<const sss::string_view&>(*this);
}
} // namespace varlisp
