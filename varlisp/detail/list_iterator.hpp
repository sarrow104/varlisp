#pragma once

#include <stdexcept>
#include <iterator>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>

#include "../object.hpp"

namespace varlisp {

namespace detail {
template <typename Out>
struct Converter {
    Converter(varlisp::List * plist) : m_plist(plist) {}
    Converter& operator=(const Out& v)
    {
        m_plist->append(v);
        COLOG_DEBUG(v);
        return *this;
    }

    varlisp::List * m_plist;
};
template <typename Out = varlisp::Object>
struct list_back_inserter_t {
    explicit list_back_inserter_t(varlisp::List& list)
        : m_list(list), m_list_ptr(nullptr)
    {
        m_list_ptr = m_list.get_slist();
        if (!m_list_ptr) {
            m_list_ptr = &m_list;
        }
    }
    list_back_inserter_t(const list_back_inserter_t& ref)
        : m_list(ref.m_list), m_list_ptr(ref.m_list_ptr)
    {
        COLOG_DEBUG(m_list_ptr, m_list_ptr->size());
    }
    ~list_back_inserter_t() = default;
    list_back_inserter_t operator++(int)
    {
        return *this;
    }
    list_back_inserter_t& operator++()
    {
        return *this;
    }
    varlisp::List& m_list;
    varlisp::List* m_list_ptr;
    Out operator*()
    {
        return m_list_ptr;
    }
};

struct list_object_const_iterator_t : std::iterator<std::forward_iterator_tag, Object>{

    explicit list_object_const_iterator_t(const varlisp::List* p_list)
        : m_list_ptr(p_list)
    {
        if (m_list_ptr) {
            // 如果是slist
            //  则枚举内部的list
            // 如果是'atom
            //  则定位到atom，即，可以取一次值
            if (m_list_ptr->is_quoted()) {
                const varlisp::List * p_slist = m_list_ptr->get_slist();
                if (p_slist) {
                    m_list_ptr = p_slist;
                    m_it = m_list_ptr->begin();
                }
                else {
                    m_it = m_list_ptr->begin() + 1;
                }
            }
        }
        COLOG_DEBUG(m_list_ptr);
    }
    list_object_const_iterator_t()
        : m_list_ptr(nullptr)
    {
        COLOG_DEBUG(m_list_ptr);
    }
    list_object_const_iterator_t(const list_object_const_iterator_t& ref)
        : m_list_ptr(ref.m_list_ptr)
    {
        COLOG_DEBUG(m_list_ptr);
    }
    bool operator!=(const list_object_const_iterator_t& ref) const
    {
        return m_list_ptr != ref.m_list_ptr || m_it != ref.m_it;
    }
    bool operator==(const list_object_const_iterator_t& ref) const
    {
        return m_list_ptr == ref.m_list_ptr && m_it == ref.m_it;
    }
    const Object& operator*() const
    {
        if (!(*this)) {
            SSS_POSITION_THROW(std::runtime_error, "nullptr");
        }
        return *m_it;
    }
    const Object* operator->() const
    {
        return &(*m_it);
    }
    operator const void * () const
    {
        if (m_list_ptr && m_it != m_list_ptr->end()) {
            return this;
        }
        else {
            return nullptr;
        }
    }
    list_object_const_iterator_t operator++(int)
    {
        list_object_const_iterator_t ret(*this);
        this->next();
        return ret;
    }
    list_object_const_iterator_t& operator++()
    {
        this->next();
        return *this;
    }
    void next()
    {
        if (*this) {
            m_it++;
        }
    }
    const varlisp::List*            m_list_ptr;
    varlisp::List::const_iterator   m_it;
};

template <typename T>
struct list_const_iterator_t {
    explicit list_const_iterator_t(const varlisp::List* p_list)
        : m_list_ptr(p_list)
    {
        if (m_list_ptr) {
            // 如果是slist
            //  则枚举内部的list
            // 如果是'atom
            //  则定位到atom，即，可以取一次值
            if (m_list_ptr->is_quoted()) {
                auto p_slist = m_list_ptr->get_slist();
                if (p_slist) {
                    m_list_ptr = p_slist;
                    m_it = m_list_ptr->begin();
                }
                else {
                    m_it = m_list_ptr->begin() + 1;
                }
            }
            else {
                m_it = m_list_ptr->begin();
            }
        }
    }
    list_const_iterator_t()
        : m_list_ptr(nullptr)
    {
    }
    list_const_iterator_t(const list_const_iterator_t& ref)
        : m_list_ptr(ref.m_list_ptr), m_it(ref.m_it)
    {
    }
    bool operator!=(const list_const_iterator_t& ref) const
    {
        return (m_list_ptr != ref.m_list_ptr || m_it != ref.m_it);
    }
    bool operator==(const list_const_iterator_t& ref) const
    {
        return m_list_ptr == ref.m_list_ptr && m_it == ref.m_it;
    }
    const T& operator*() const
    {
        if (!is_ok()) {
            SSS_POSITION_THROW(std::runtime_error, "nullptr");
        }
        const T* p_val = boost::get<T>(&*m_it);
        if (!p_val) {
            SSS_POSITION_THROW(std::runtime_error, *m_it);
        }
        return *p_val;
    }
    bool is_ok() const
    {
        if (m_list_ptr) {
            if (m_it == varlisp::List::const_iterator()) {
                SSS_POSITION_THROW(std::runtime_error, "not init iterator");
            }
            return size_t(std::distance(m_list_ptr->begin(), m_it)) < m_list_ptr->size();
        }
        return false;
    }
    operator const void * () const
    {
        if (is_ok()) {
            return this;
        }
        else {
            return nullptr;
        }
    }
    list_const_iterator_t operator++(int)
    {
        list_const_iterator_t ret(*this);
        this->next();
        return ret;
    }
    list_const_iterator_t& operator++()
    {
        this->next();
        return *this;
    }
    void clear()
    {
        m_list_ptr = nullptr;
        m_it = varlisp::List::const_iterator();
    }
    void next()
    {
        if (*this) {
            m_it++;
            if (m_it == m_list_ptr->end()) {
                clear();
            }
        }
        else {
            clear();
        }
    }
    const varlisp::List* m_list_ptr;
    varlisp::List::const_iterator   m_it;
};

template <typename Out>
list_back_inserter_t<Converter<Out>> list_back_inserter(varlisp::List& list)
{
    return list_back_inserter_t<Converter<Out>>(list);
}

}  // namespace detail

}  // namespace varlisp
