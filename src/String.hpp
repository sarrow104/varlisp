#pragma once

#include <memory>
#include <iosfwd>

#include <sss/string_view.hpp>

#include <re2/re2.h>

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

    String(sss::string_view s, bool )
        : sss::string_view(s)
    {
    }

    String& assign(sss::string_view s, bool )
    {
        this->clear();
        this->sss::string_view::operator=(s);
        return *this;
    }

protected:
    String(sss::string_view s, std::shared_ptr<std::string> ref)
        : sss::string_view(s), m_refer(ref)
    {
    }

public:
    std::shared_ptr<std::string> gen_shared() const;

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
    const char * safe_c_str() const;

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

    String substr(const sss::string_view& s) const
    {
        return substr(s.data() - this->data(), s.size());
    }

    sss::string_view to_string_view() const
    {
        return {this->data(), this->size()};
    }

    operator re2::StringPiece() const
    {
        return {this->data(), this->size()};
    }

    size_t use_count() const
    {
        return this->m_refer.use_count();
    }

private:
    std::shared_ptr<std::string> m_refer;
};

typedef String string_t;

inline std::ostream& operator<<(std::ostream& o, const String& s)
{
    s.print(o);
    return o;
}
}  // namespace varlisp
