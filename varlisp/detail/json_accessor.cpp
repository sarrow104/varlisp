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

const Object * json_accessor::access(const Environment& env) const
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

Object * json_accessor::access(Environment& env) const
{
    return const_cast<Object*>(this->access(const_cast<const Environment&>(env)));
}

const Object * json_accessor::find_name(const Object* obj, size_t id) const
{
    const Environment * p_env = boost::get<varlisp::Environment>(obj);
    // TODO FIXME
    // 从access 到 find_name，内部连续调用了Environment::find()；而
    // Environment::find()，本身就是一个递归的查找——这在逻辑上有些错误了！
    // 我本意是查找手动形成的{}嵌套关系！
    // 而手动创建的{}嵌套关系，只能从外部找内部——内部则并没有到外部的parent的
    // 关系，因此本程序虽然通过递归版的Environment::find()查找标识符，但是工作仍
    // 然正常——逻辑不对，但是结果正确。
    // 解决办法是：
    // 1. Environment::find() 改名为更确定的Environment::deep_find()；
    // 2. 本函数使用map::find()来完成查找；
    // 3. 修改Environment的赋值动作，创建链接关系；
    const Object * p_ret = p_env->find(m_stems[id]);
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

} // namespace detail
} // namespace varlisp
