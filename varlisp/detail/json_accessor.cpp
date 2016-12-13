#include "json_accessor.hpp"

#include <cctype>

#include <sss/algorithm.hpp>
#include <sss/spliter.hpp>
#include <sss/colorlog.hpp>

#include "../environment.hpp"
#include "../list.hpp"

namespace varlisp {
namespace detail {

json_accessor::json_accessor(const std::string& jstyle_name)
    : m_jstyle_name(jstyle_name), m_colon_pos(m_jstyle_name.find(':'))
{
    if (m_jstyle_name.find("::") != std::string::npos)
    {
        SSS_POSITION_THROW(std::runtime_error, m_jstyle_name,
                           "double :: found! ");
    }
    if (jstyle_name.empty()) {
        SSS_POSITION_THROW(std::runtime_error, "empty name");
    }
    if (jstyle_name.front() == ':') {
        SSS_POSITION_THROW(std::runtime_error, m_jstyle_name, "start with :");
    }
    if (jstyle_name.back() == ':') {
        SSS_POSITION_THROW(std::runtime_error, m_jstyle_name, "end with :");
    }

    if (this->has_sub()) {
        sss::Spliter sp(this->tail(), ':');
        std::string stem;
        m_stems.clear();
        while(sp.fetch_next(stem)) {
            m_stems.emplace_back(std::move(stem));
        }
    }
}

const varlisp::Object * json_accessor::access(const varlisp::Environment& env) const
{
    const Object * p_obj = env.deep_find(this->prefix());
    if (m_stems.empty() || !p_obj) {
        return const_cast<Object*>(p_obj);
    }
    
    // NOTE 执行环境的Environment的parent()继承关系，
    // 与名字name的":"拆分，是没有关系的！
    // 因为，环境的继承，其实是调用了函数，或者使用了let等等，
    // 可以创建局部作用域的语句。
    // 此时，获取对象的语义规则，是没变的！
    //
    // 假设名字 a:10。
    // 我的想法是，这是获取一个名为a的slist对象，然后取10的元素。
    // 这貌似不方便递归完成。
    //
    // a:10:b
    //
    // 这样，又要求这个数组元素，还是一个{}；
    //
    // 另外，要求list的index，必须连用！
    // 比如："a:3:4:b"；其中的3:4，表示数组的数组；
    //
    // 即，要分开！因为3,4这里的查找，是在List中完成！
    // 而具名的查找，是在Environment中完成；
    //
    // 因此，还是将动作，派发给外部对象吧！
    if (sss::is_all(m_stems.front(), static_cast<int(*)(int)>(std::isdigit)))
    {
        return this->find_index(p_obj, 0);
    }
    else {
        return this->find_name(p_obj, 0);
    }
}

varlisp::Object * json_accessor::access(varlisp::Environment& env) const
{
    return const_cast<varlisp::Object*>(this->access(const_cast<const varlisp::Environment&>(env)));
}

const varlisp::Object * json_accessor::find_name(const varlisp::Object* obj, size_t id) const
{
    const varlisp::Environment * p_env = boost::get<varlisp::Environment>(obj);
    if (!p_env) {
        SSS_POSITION_THROW(std::runtime_error,
                           obj->which(), " is not a Environment");
    }
    const varlisp::Object * p_ret = p_env->find(m_stems[id]);
    if (!p_ret || id + 1 == m_stems.size()) {
        return p_ret;
    }
    if (sss::is_all(m_stems[id + 1], static_cast<int(*)(int)>(std::isdigit))) {
        return this->find_index(p_ret, id + 1);
    }
    else {
        return find_name(p_ret, id + 1);
    }
}

Object * json_accessor::find_name(Object* obj, size_t id) const
{
    return const_cast<Object*>(this->find_name(const_cast<const Object*>(obj), id));
}

const Object * json_accessor::find_index(const Object * obj, size_t id) const
{
    const List * p_list = boost::get<varlisp::List>(obj);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error,
                           obj->which(), " is not a list");
    }
    int index = sss::string_cast<int>(m_stems[id]);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error,
                           "require index ", index, ", not from a list;");
    }
 
    const Object * p_ret = p_list->objAt(index);
    if (!p_ret || id + 1 == m_stems.size()) {
        return p_ret;
    }
    if (sss::is_all(m_stems[id + 1], static_cast<int(*)(int)>(std::isdigit))) {
        return this->find_index(p_ret, id + 1);
    }
    else {
        return find_name(p_ret, id + 1);
    }
}

Object * json_accessor::find_index(Object * obj, size_t id) const
{
    return const_cast<Object*>(this->find_index(const_cast<const Object*>(obj), id));
}

namespace detail {
std::pair<const varlisp::Object*, const varlisp::Environment*> locate_impl(const varlisp::Environment& env, const std::string& name)
{
    auto * p_env = &env;
    while (p_env) {
        auto * p_obj = p_env->find(name);
        if (p_obj) {
            return std::make_pair(p_obj, p_env);
        }
        p_env = p_env->parent();
    }
    return {nullptr, nullptr};
}
} // namespace detail

std::pair<const varlisp::Object*, const varlisp::Environment*> json_accessor::locate(const varlisp::Environment& env, const symbol& sym)
{
    json_accessor jc(sym.name());
    if (!jc.has_sub()) {
        return detail::locate_impl(env, sym.name());
    }
    else {
        auto location = detail::locate_impl(env, jc.prefix());
        for (size_t i = 0; i < jc.m_stems.size() && location.first; ++i) {
            if (sss::is_all(jc.m_stems[i], static_cast<int(*)(int)>(std::isdigit))) {
                auto * p_list = boost::get<varlisp::List>(location.first);
                if (!p_list) {
                    SSS_POSITION_THROW(std::runtime_error,
                                       "need a List here , but ", location.first->which());
                }

                location.first = p_list->objAt(sss::string_cast<int>(jc.m_stems[i]));
            }
            else {
                // auto * p_env = location.second
                location.second = boost::get<varlisp::Environment>(location.first);
                if (!location.second) {
                    SSS_POSITION_THROW(std::runtime_error,
                                       "need an Environment here , but ", location.first->which());
                }
                location.first = location.second->find(jc.m_stems[i]);
            }
        }
        return location;
    }
}

std::pair<varlisp::Object*, varlisp::Environment*> json_accessor::locate(varlisp::Environment& env, const varlisp::symbol& sym)
{
    auto ret = json_accessor::locate(const_cast<const varlisp::Environment&>(env), sym);
    return std::make_pair(const_cast<varlisp::Object*>(std::get<0>(ret)), const_cast<varlisp::Environment*>(std::get<1>(ret)));
}

} // namespace detail
} // namespace varlisp
