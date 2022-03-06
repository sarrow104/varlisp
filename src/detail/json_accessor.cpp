#include "json_accessor.hpp"

#include <cctype>
#include <cstddef>
#include <cstdlib>

#include <sss/algorithm.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/spliter.hpp>

#include "../environment.hpp"
#include "../list.hpp"

namespace varlisp::detail {

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
    if (is_index(m_stems.front()))
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
    if (is_index(m_stems[id + 1])) {
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
    if (p_list->is_quoted()) {
        p_list = p_list->unquoteType<varlisp::List>();
    }
    int index = sss::string_cast<int>(m_stems[id]);
    if (!p_list) {
        SSS_POSITION_THROW(std::runtime_error,
                           "require index ", index, ", not from a list;");
    }
    // COLOG_ERROR(SSS_VALUE_MSG(index));
    if (index < 0) {
        index += p_list->size();
    }
    if (index < 0) {
        return nullptr;
    }
    // COLOG_ERROR(SSS_VALUE_MSG(p_list->size()), SSS_VALUE_MSG(index));

    const Object * p_ret = &p_list->nth(index);
    if (id + 1 == m_stems.size()) {
        return p_ret;
    }
    if (is_index(m_stems[id + 1])) {
        return this->find_index(p_ret, id + 1);
    }
    else {
        return find_name(p_ret, id + 1);
    }
}

// NOTE 函数逻辑：
// 1. 在env直接查找prefix；如果没有sub
// 2. 根据下标类型(是否整数)，分别进入不同的子函数；query_index 和 query_field;
// 3. 如果进入query_field，那么要求当前是Env类型；如果类型不同，则重新赋值它为Env，然后添加值为nil
//    的该名称field ；返回该field的引用
// 4. 如果进入query_index，那么要求当前是Slist；如果类型不同，则生成一个可以容纳该index的Slist；
//    并且，所有元素都是nil；然后返回该index的元素的引用；另外，如果index是负数，那么，取abs后，
//    就是新数组的长度——如果需要创建的话。
// NOTE Environment::operator[]()是在当前查找prefix！即，find()而不是 deep_find()!
//    但是，我还想让这个函数，支持(setf)——它默认就是针对当前环境——但问题是，它deep_find()吗？
//    define呢？默认是针对top_env()
Object& json_accessor::query_location(varlisp::Environment& env) const
{
    Object * p_obj = env.deep_find(this->prefix());
    if (!p_obj) {
        env[this->prefix()] = Nill{};
        p_obj = env.find(this->prefix());
    }
    if (!this->has_sub()) {
        return *p_obj;
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
    if (is_index(m_stems.front()))
    {
        return this->query_index(*p_obj, 0);
    }
    else {
        return this->query_field(*p_obj, 0);
    }

}

Object& json_accessor::query_field(Object& obj, size_t id) const
{
    varlisp::Environment * p_env = boost::get<varlisp::Environment>(&obj);
    if (!p_env) {
        obj = varlisp::Environment();
        p_env = boost::get<varlisp::Environment>(&obj);
    }
    varlisp::Object * p_ret = p_env->find(m_stems[id]);
    if (!p_ret) {
        p_env->operator[](m_stems[id]) = Nill{};
        p_ret = p_env->find(m_stems[id]);
    }
    if (id + 1 == m_stems.size()) {
        return *p_ret;
    }
    if (is_index(m_stems[id + 1])) {
        return this->query_index(*p_ret, id + 1);
    }
    else {
        return this->query_field(*p_ret, id + 1);
    }
}

Object& json_accessor::query_index(Object& obj, size_t id) const
{
    List * p_list = boost::get<varlisp::List>(&obj);
    int index = sss::string_cast<int>(m_stems[id]);
    size_t abs_size = std::abs(index);
    if (!p_list) {
        auto List = varlisp::List();
        for (size_t i = 0; i < abs_size; ++i) {
            List.append(Nill{});
        }
        obj = varlisp::List::makeSQuoteObj(List);
        p_list = boost::get<varlisp::List>(&obj);
    }
    if (p_list->is_quoted()) {
        p_list = p_list->unquoteType<varlisp::List>();
    }
    if (!p_list) {
        auto List = varlisp::List();
        for (size_t i = 0; i < abs_size; ++i) {
            List.append(Nill{});
        }
        obj = varlisp::List::makeSQuoteObj(List);
        p_list = boost::get<varlisp::List>(&obj);
        p_list = p_list->unquoteType<varlisp::List>();
    }
    if (p_list->size() < abs_size) {
        for (size_t i = 0; i < abs_size - p_list->size(); ++i) {
            p_list->append(Nill{});
        }
    }
    // COLOG_ERROR(SSS_VALUE_MSG(index));
    if (index < 0) {
        index += p_list->size();
    }
    if (index < 0) {

        SSS_POSITION_THROW(std::runtime_error, "must not negative");
    }
    // COLOG_ERROR(SSS_VALUE_MSG(p_list->size()), SSS_VALUE_MSG(index));

    Object * p_ret = &p_list->nth(index);
    if (id + 1 == m_stems.size()) {
        return *p_ret;
    }
    if (is_index(m_stems[id + 1])) {
        return this->query_index(*p_ret, id + 1);
    }
    else {
        return this->query_field(*p_ret, id + 1);
    }
}

Object * json_accessor::find_index(Object * obj, size_t id) const
{
    return const_cast<Object*>(this->find_index(const_cast<const Object*>(obj), id));
}

namespace detail {
std::pair<const varlisp::Object*, const varlisp::Environment*> locate_impl(const varlisp::Environment& env, const std::string& name)
{
    COLOG_DEBUG(&env, name);
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

json_accessor::location json_accessor::locate(varlisp::Environment& env)
{
    json_accessor& jc{*this};
    auto pl = detail::locate_impl(env, jc.prefix());
    varlisp::List* parentList = nullptr;

    if (!jc.has_sub()) {
        return {const_cast<varlisp::Object*>(pl.first), const_cast<varlisp::Environment*>(pl.second), nullptr};
    }
    COLOG_DEBUG(pl);
    for (size_t i = 0; i < jc.m_stems.size() && pl.first; ++i) {
        COLOG_DEBUG(jc.m_stems[i], is_index(jc.m_stems[i]));
        if (is_index(jc.m_stems[i])) {
            auto * p_list = boost::get<varlisp::List>(pl.first);
            if (!p_list) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "need a List here , but ", pl.first->which());
            }
            parentList = const_cast<varlisp::List*>(p_list);
            if (p_list->is_quoted()) {
                p_list = p_list->unquoteType<varlisp::List>();
                if (!p_list) {
                    SSS_POSITION_THROW(std::runtime_error,
                                       "quoted but not s-list", *pl.first);
                }
            }

            int index = sss::string_cast<int>(jc.m_stems[i]);
            if (std::abs(index) >= p_list->size()) {
                SSS_POSITION_THROW(std::runtime_error, "require ", index,
                                   "th element from ", p_list->size());
            }
            if (index < 0) {
                index += p_list->size();
            }

            pl.first = &p_list->nth(index);
            // pl.first = p_list->objAt(sss::string_cast<int>(jc.m_stems[i]));
            COLOG_DEBUG(*p_list);
        }
        else {
            // auto * p_env = pl.second
            pl.second = boost::get<varlisp::Environment>(pl.first);
            if (!pl.second) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "need an Environment here , but ", pl.first->which());
            }
            parentList = nullptr;
            pl.first = pl.second->find(jc.m_stems[i]);
        }
        COLOG_DEBUG(pl);
    }

    if (parentList != nullptr) {
        return {const_cast<varlisp::Object*>(pl.first), nullptr, parentList};
    }
    return {const_cast<varlisp::Object*>(pl.first), const_cast<varlisp::Environment*>(pl.second), nullptr};
}

} // namespace varlisp::detail
