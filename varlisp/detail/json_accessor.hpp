#include <string>
#include <vector>
#include <stdexcept>

#include <sss/util/PostionThrow.hpp>

#include "../object.hpp"

namespace varlisp {
struct Environment;
struct List;
namespace detail {
class json_accessor
{
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

    static std::pair<const varlisp::Object*, const varlisp::Environment*> locate(const varlisp::Environment& env, const varlisp::symbol& sym);
    static std::pair<varlisp::Object*, varlisp::Environment*> locate(varlisp::Environment& env, const varlisp::symbol& sym);

private:
    const Object * find_name(const Object* obj, size_t id) const;
    const Object * find_index(const Object* list, size_t id) const;
    Object * find_name(Object* obj, size_t id) const;
    Object * find_index(Object* list, size_t id) const;
};
} // namespace detail
} // namespace varlisp
