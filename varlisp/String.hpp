#pragma once

#include <memory>
#include <sss/string_view.hpp>

#include <iosfwd>

namespace varlisp {

struct String : public sss::string_view {
public:
    String() = default;
    ~String() = default;

    explicit String(const std::string& s)
    {
        if (!s.empty()) {
            m_refer = std::make_shared<std::string>(s);
            sss::string_view::operator=(*m_refer);
        }
    }
    template <size_t N>
    String(const char (&s)[N]) : sss::string_view(s)
    {
    }

    String(const String& ref) = default;
    String& operator=(const String& ref) = default;
    String(String&& ref) = default;
    String& operator=(String&& ref) = default;

    String& operator=(const std::string&);
    String& operator=(std::string&&);

    String(sss::string_view s, std::shared_ptr<std::string> ref)
        : sss::string_view(s), m_refer(ref)
    {
    }

public:
    bool own_text() const {
        if (this->m_refer) {
            const char * buf = this->m_refer->data();
            size_t len = this->m_refer->length();
            return (buf && this->data() >= buf && this->data() < (buf + len));
        }
        return false;
    }
    const char * c_str() const {
        return this->data();
    }
    void print(std::ostream& o) const;
    void clear()
    {
        sss::string_view::clear();
        m_refer.reset();
    }
    String substr(size_t offset) const
    {
        String ret{sss::string_view::substr(offset), m_refer};
        if (ret.empty()) {
            ret.clear();
        }
        return ret;
    }
    String substr(size_t offset, size_t len) const
    {
        String ret{sss::string_view::substr(offset, len), m_refer};
        if (ret.empty()) {
            ret.clear();
        }
        return ret;
    }

private:
    std::shared_ptr<std::string> m_refer;
};

inline std::ostream& operator<<(std::ostream& o, const String& s)
{
    s.print(o);
    return o;
}
}  // namespace varlisp
