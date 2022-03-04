#include "environment.hpp"

#include <cctype>

#include <sss/log.hpp>
#include <sss/util/Memory.hpp>
#include <sss/colorlog.hpp>
#include <sss/algorithm.hpp>
#include <sss/debug/value_msg.hpp>

#include "eval_visitor.hpp"
#include "detail/json_accessor.hpp"

namespace varlisp {

Environment::Environment(Environment* parent)
    : m_parent(parent)
{
    COLOG_DEBUG(this, "from", parent);
}

Environment::~Environment()
{
    while(!m_defer_task.empty()) {
        const Object& task = m_defer_task.back();
        try {
            boost::apply_visitor(eval_visitor(*this), m_defer_task.back());
        }
        catch (std::exception& e) {
            COLOG_ERROR(e.what());
        }
        catch (...)
        {
            COLOG_DEBUG("unkown exception while eval", task);
        }
        m_defer_task.pop_back();
    }
}

void   Environment::defer_task_push(const Object& task)
{
    m_defer_task.push_back(task);
}

void   Environment::defer_task_push(Object&& task)
{
    m_defer_task.emplace_back(std::move(task));
}

size_t Environment::defer_task_size() const
{
    return m_defer_task.size();
}

void   Environment::print(std::ostream& o) const
{
    o << '{';

    for (auto it = this->BaseT::begin(); it != this->BaseT::end(); ++it) {
        o << '(' << it->first << ' ';
        boost::apply_visitor(print_visitor(o), it->second.first);
        o << ')';
    }
    o << '}';
}

// FIXME 改名！
// deep_find 1. 考虑paent，2.考虑了json_accessor ； 而类似名称的find()仅考虑了当前层
//
// 也就是说，函数命名不规范。最好更换API名字。
// local_find,deep_find,json_access等等
// 从逻辑上，find应该也支持json_access，只不过只针对当前环境。
// 这样，deep_find，就可以写得简单。
const Object* Environment::deep_find(const std::string& name) const
{
    detail::json_accessor jc(name);
    COLOG_DEBUG(SSS_VALUE_MSG(name), SSS_VALUE_MSG(jc.has_sub()));
    if (!jc.has_sub()) {
        const Environment* pe = this;
        const Object* ret = 0;
        do {
            auto it = pe->BaseT::find(name);
            if (it != pe->BaseT::cend()) {
                ret = &it->second.first;
            }
            pe = pe->m_parent;
        } while (pe && !ret);

        return ret;
    }
    else {
        return jc.access(*this);
    }
}

Object* Environment::deep_find(const std::string& name)
{
    return const_cast<Object*>(const_cast<const Environment*>(this)->deep_find(name));
}

const Object* Environment::find(const std::string& name) const
{
    const Object* ret = nullptr;
    auto it = this->BaseT::find(name);
    if (it != this->BaseT::end()) {
        ret = &it->second.first;
    }
    return ret;
}

Object* Environment::find(const std::string& name)
{
    return const_cast<Object*>(const_cast<const Environment*>(this)->find(name));
}

// TODO erase 这里，也有些不合适。undef函数内部，使用json_accessor完成深度定位方式的删除。
// 这部分逻辑，应该添加到Environment这里。或者额外添加一个visitor类。
bool Environment::erase(const std::string& name)
{
    // FIXME 貌似不能erase子对象，只能整个删除
    // 因为是往上查找的
    bool erased = false;
    Environment* pe = this;
    do {
        auto it = pe->BaseT::find(name);
        if (it != pe->BaseT::end()) {
            if (it->second.second.is_const) {
                SSS_POSITION_THROW(std::runtime_error,
                                   "cannot erase const Object ", it->second.first);
            }
            pe->BaseT::erase(it);
            erased = true;
        }
        pe = pe->m_parent;
    } while (pe && !erased);
    return erased;
}

Environment * Environment::ceiling(){
    Environment * p_curent = this;
    while (p_curent && p_curent->m_parent) {
        p_curent = p_curent->m_parent;
    }
    return p_curent;
}

// TODO 增加一个wrapper class；
// 当完成赋值动作的时候，重建链接关系；
Object& Environment::operator [](const std::string& name)
{
    detail::json_accessor jc(name);
    if (!jc.has_sub()) {
        return this->BaseT::operator[](name).first;
    }
    else {
        return jc.query_location(*this);
        // TODO
        // 检测使用json_accessor 的curd操作
        //
        // std::string env_name = jc.prefix();
        // auto it = this->BaseT::find(env_name);
        // if (it == this->BaseT::end()) {
        //     // FIXME 这里，提前假定类型是Environment()——
        //     // 要知道，也有可能是list!
        //     // 因此，应该通过下一层"下标"的具体值，来决定如何插入。
        //     it = this->BaseT::emplace_hint(it,
        //                                    std::move(env_name),
        //                                    std::make_pair(std::move(Environment(this)), std::move(varlisp::property_t(false)))); 
        //     if (it == this->BaseT::end()) {
        //         SSS_POSITION_THROW(std::runtime_error, "insert errror!");
        //     }
        // }
        // Environment * p_inner = boost::get<Environment>(&it->second.first);
        // if (!p_inner) {
        //     it->second.first = Environment(this);
        //     p_inner = boost::get<Environment>(&it->second.first);
        //     if (!p_inner) {
        //         SSS_POSITION_THROW(std::runtime_error, "assignment error null");
        //     }
        // }
        // return p_inner->operator[](jc.tail());
    }
}

size_t Environment::clear(bool is_force)
{
    auto it = this->BaseT::begin();
    size_t cnt = 0;
    while (it != this->BaseT::end()) {
        if (is_force || !it->second.second.is_const) {
            it = this->BaseT::erase(it);
            ++cnt;
        }
        else {
            ++it;
        }
    }
    return cnt;
}

void Environment::insert(const std::string& name, const Object& o, bool is_const)
{
    auto it = this->BaseT::find(name);
    if (it == this->BaseT::end()) {
        this->BaseT::insert(
            it, std::make_pair(
                name, std::make_pair(o, varlisp::property_t(is_const))));
    }
}

void Environment::insert(std::string&& name, Object&& o, bool is_const)
{
    auto it = this->BaseT::find(name);
    if (it == this->BaseT::end()) {
        this->BaseT::emplace_hint(
            it, std::move(name),
            std::make_pair(std::move(o),
                           std::move(varlisp::property_t(is_const))));
    }
}

bool Environment::operator == (const Environment& env) const
{
    // TODO
    return false;
}

bool Environment::operator < (const Environment& env) const
{
    // TODO
    return false;
}

}  // namespace varlisp
