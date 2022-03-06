#include <string>
#include <vector>
#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "../object.hpp"

namespace varlisp {
struct Environment;
struct List;
namespace detail {

inline bool is_index(const std::string& stem)
{
    if (!stem.empty()) {
        auto it_beg = stem.begin();
        if (*it_beg == '-' && stem.length() >= 2) {
            ++it_beg;
        }
        return sss::is_all(it_beg, stem.end(), static_cast<int(*)(int)>(std::isdigit));
    }
    return false;
}

class json_accessor
{
    struct location {
        varlisp::Object*      obj; // located object by accessor
        varlisp::Environment* env; // none-nullptr when reference by symbol
        varlisp::List*        list; // none-nullptr when reference by index
    };
    const std::string&          m_jstyle_name;
    std::vector<std::string>    m_stems;
    std::string::size_type      m_colon_pos;

public:
    json_accessor(const std::string& jstyle_name);
    ~json_accessor() = default;

public:
    bool has_sub() const {
        return m_colon_pos != std::string::npos;
    }
    std::string prefix() const {
        if (has_sub()) {
            return m_jstyle_name.substr(0, m_colon_pos);
        }
        else {
            return m_jstyle_name;
        }
    }
    std::string tail() const {
        if (has_sub()) {
            return m_jstyle_name.substr(m_colon_pos + 1);
        }
        else {
            return "";
        }
    }

    const std::vector<std::string>& stems() const {
        return m_stems;
    }

    const varlisp::Object * access(const varlisp::Environment& env) const;
    varlisp::Object *       access(varlisp::Environment& env) const;
    varlisp::Object&        query_location(varlisp::Environment& env) const;

    location locate(varlisp::Environment& env);

private:
    Object& query_field(Object& obj, size_t id) const;
    Object& query_index(Object& obj, size_t id) const;
    const Object * find_name(const Object* obj, size_t id) const;
    const Object * find_index(const Object* list, size_t id) const;
    Object * find_name(Object* obj, size_t id) const;
    Object * find_index(Object* list, size_t id) const;
};

// TODO eval_locate的代码，也与access，这部分代码，重复了，应该合并。
// 说白了，就是更改access内部接口。提供更方便的——
} // namespace detail
} // namespace varlisp
