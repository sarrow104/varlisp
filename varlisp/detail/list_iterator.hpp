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
    Converter(Object& obj) : m_obj(obj) {}
    Converter& operator=(const Out& v)
    {
        m_obj = v;
        COLOG_DEBUG(v);
        return *this;
    }

    Object& m_obj;
};
template <typename Out = varlisp::Object>
struct list_back_inserter_t {
    explicit list_back_inserter_t(varlisp::List& list)
        : m_list(list), m_list_ptr(nullptr)
    {
        m_list_ptr = &m_list;
        while (m_list_ptr && m_list_ptr->head.which()) {
            COLOG_DEBUG(m_list_ptr, m_list_ptr->head.which(), m_list_ptr->head);
            m_list_ptr = m_list_ptr->next_slot();
        }
    }
    list_back_inserter_t(const list_back_inserter_t& ref)
        : m_list(ref.m_list), m_list_ptr(ref.m_list_ptr)
    {
        COLOG_DEBUG(m_list_ptr, m_list_ptr->head.which(), m_list_ptr->head);
    }
    ~list_back_inserter_t() = default;
    list_back_inserter_t operator++(int)
    {
        list_back_inserter_t ret(*this);
        this->next();
        COLOG_DEBUG(ret.m_list_ptr, m_list_ptr);
        return ret;
    }
    list_back_inserter_t& operator++()
    {
        varlisp::List* p_l = m_list_ptr;
        this->next();
        COLOG_DEBUG(p_l, m_list_ptr);
        return *this;
    }
    void next() { m_list_ptr = m_list_ptr->next_slot(); }
    varlisp::List& m_list;
    varlisp::List* m_list_ptr;
    Out operator*()
    {
        COLOG_DEBUG(m_list_ptr, m_list_ptr->head.which());
        return m_list_ptr->head;
    }
};

struct list_object_const_iterator_t : std::iterator<std::forward_iterator_tag, Object>{

    explicit list_object_const_iterator_t(const varlisp::List* p_list)
        : m_list_ptr(p_list)
    {
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
        return m_list_ptr != ref.m_list_ptr;
    }
    bool operator==(const list_object_const_iterator_t& ref) const
    {
        return m_list_ptr == ref.m_list_ptr;
    }
    const Object& operator*() const
    {
        if (!(*this)) {
            SSS_POSITION_THROW(std::runtime_error, "nullptr");
        }
        return m_list_ptr->head;
    }
    const Object* operator->() const
    {
        return &m_list_ptr->head;
    }
    operator const void * () const
    {
        if (m_list_ptr && m_list_ptr->head.which()) {
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
        m_list_ptr = m_list_ptr->next();
        if (!m_list_ptr->head.which()) {
            m_list_ptr = 0;
        }
    }
    const varlisp::List* m_list_ptr;
};

template <typename T>
struct list_const_iterator_t {
    explicit list_const_iterator_t(const varlisp::List* p_list)
        : m_list_ptr(p_list)
    {
        COLOG_DEBUG(m_list_ptr);
    }
    list_const_iterator_t()
        : m_list_ptr(nullptr)
    {
        COLOG_DEBUG(m_list_ptr);
    }
    list_const_iterator_t(const list_const_iterator_t& ref)
        : m_list_ptr(ref.m_list_ptr)
    {
        COLOG_DEBUG(m_list_ptr);
    }
    bool operator!=(const list_const_iterator_t& ref) const
    {
        return m_list_ptr != ref.m_list_ptr;
    }
    bool operator==(const list_const_iterator_t& ref) const
    {
        return m_list_ptr == ref.m_list_ptr;
    }
    const T& operator*() const
    {
        if (!m_list_ptr) {
            SSS_POSITION_THROW(std::runtime_error, "nullptr");
        }
        const T* p_val = boost::get<T>(&m_list_ptr->head);
        if (!p_val) {
            SSS_POSITION_THROW(std::runtime_error, m_list_ptr->head.which(),
                               m_list_ptr->head);
        }
        return *p_val;
    }
    operator const void * () const
    {
        if (m_list_ptr && m_list_ptr->head.which()) {
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
    void next()
    {
        m_list_ptr = m_list_ptr->next();
        if (!m_list_ptr->head.which()) {
            m_list_ptr = 0;
        }
    }
    const varlisp::List* m_list_ptr;
};

template <typename Out>
list_back_inserter_t<Converter<Out>> list_back_inserter(varlisp::List& list)
{
    return list_back_inserter_t<Converter<Out>>(list);
}

}  // namespace detail

}  // namespace varlisp
