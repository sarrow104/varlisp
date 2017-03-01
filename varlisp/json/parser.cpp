#include "parser.hpp"

#include <cctype>
#include <stdexcept>

#include <sss/util/PostionThrow.hpp>
#include <sss/colorlog.hpp>
#include <sss/util/utf8.hpp>
#include <sss/jsonpp/parser.hpp>

#include "../detail/list_iterator.hpp"

namespace varlisp {
namespace json {

struct LispHandle
{
    Object& m_result;
    std::vector<Object*> m_stack_obj;
    Object* m_kv_value;
    varlisp::List* m_list;
    explicit LispHandle(Object& result)
        : m_result(result), m_kv_value(nullptr), m_list(nullptr)
    {}
    bool Null()
    {
        if (m_kv_value) {
            *m_kv_value = Nill{};
            m_kv_value = nullptr;
        }
        else if (m_list){
            m_list->append(Nill{});
        }
        else {
            SSS_POSITION_THROW(std::runtime_error, "wrong");
        }
        return true;
    }
    bool Bool(bool v)
    {
        if (m_kv_value) {
            *m_kv_value = v;
            m_kv_value = nullptr;
        }
        else if (m_list){
            m_list->append(v);
        }
        else {
            SSS_POSITION_THROW(std::runtime_error, "wrong");
        }
        return true;
    }
    bool Int64(int64_t v)
    {
        if (m_kv_value) {
            *m_kv_value = v;
            m_kv_value = nullptr;
        }
        else if (m_list){
            m_list->append(v);
        }
        else {
            SSS_POSITION_THROW(std::runtime_error, "wrong");
        }
        return true;
    }
    bool Double(double v)
    {
        if (m_kv_value) {
            *m_kv_value = v;
            m_kv_value = nullptr;
        }
        else if (m_list){
            m_list->append(v);
        }
        else {
            SSS_POSITION_THROW(std::runtime_error, "wrong");
        }
        return true;
    }
    // bool RawNumber(const Ch* str, SizeType length, bool copy);
    bool String(sss::string_view v)
    {
        if (m_kv_value) {
            *m_kv_value = varlisp::string_t(v.to_string());
            m_kv_value = nullptr;
        }
        else if (m_list){
            m_list->append(varlisp::string_t(v.to_string()));
        }
        else {
            SSS_POSITION_THROW(std::runtime_error, "wrong");
        }
        return true;
    }
    bool StartObject()
    {
        if (m_stack_obj.empty()) {
            m_stack_obj.push_back(&m_result);
            (*m_stack_obj.back()) = varlisp::Environment();
        }
        else {
            if (m_kv_value) {
                *m_kv_value = varlisp::Environment();
                m_stack_obj.push_back(m_kv_value);
                m_kv_value = nullptr;
            }
            else if (m_list){
                m_list->append(varlisp::Environment());
                m_stack_obj.push_back(&m_list->nth(m_list->size() - 1));
            }
            else {
                SSS_POSITION_THROW(std::runtime_error, "wrong");
            }
        }
        return true;
    }
    bool Key(sss::string_view s)
    {
        std::string name = s.to_string();
        auto p_env = boost::get<varlisp::Environment>(m_stack_obj.back());
        p_env->operator[](name) = Nill{};
        m_kv_value = &p_env->operator[](name);
        return true;
    }
    bool EndObject(int )
    {
        m_stack_obj.pop_back();
        return true;
    }
    bool StartArray()
    {
        if (m_stack_obj.empty()) {
            m_stack_obj.push_back(&m_result);
            (*m_stack_obj.back()) = varlisp::List::makeSQuoteList();
        }
        else {
            if (m_kv_value) {
                *m_kv_value = varlisp::List::makeSQuoteList();
                m_stack_obj.push_back(m_kv_value);
                m_kv_value = nullptr;
            }
            else if (m_list){
                m_list->append(varlisp::List::makeSQuoteList());
                m_stack_obj.push_back(&m_list->nth(m_list->size() - 1));
            }
            else {
                SSS_POSITION_THROW(std::runtime_error, "wrong");
            }
            m_list = boost::get<varlisp::List>(m_stack_obj.back())->get_slist();
        }
        return true;
    }
    bool EndArray(int )
    {
        m_stack_obj.pop_back();
        return false;
    }
};

struct JParser
{
    sss::json::Parser m_p;
    JParser(sss::string_view s, Object& ret)
    {
        sss::string_view s_bak = s;
        try {
            parse(s, ret);
            this->skip_white_space(s);
            if (!s.empty()) {
                if (ret.which()) {
                    SSS_POSITION_THROW(std::runtime_error, "extra char:",
                                       sss::raw_char(s.front()));
                }
                else {
                    SSS_POSITION_THROW(std::runtime_error, "error bytes:",
                                       sss::raw_char(s.front()));
                }
            }
        }
        catch (...)
        {
            COLOG_ERROR(sss::string_view(s_bak.begin(), s.begin() - s_bak.begin()));
            throw;
        }
    }
    bool parse(sss::string_view& s, Object& ret)
    {
        COLOG_DEBUG(s);
        this->skip_white_space(s);
        if (s.empty()) {
            SSS_POSITION_THROW(std::runtime_error, "empty string");
        }
        switch(m_p.peekType(s)) {
            case sss::json::Parser::kJE_OBJECT_START:
                parse_object(s, ret);
                return true;

            case sss::json::Parser::kJE_ARRAY_START:
                parse_array(s, ret);
                return true;

            default:
                COLOG_DEBUG(s);
                return false;
        }
    }

    void parse_value(sss::string_view& s, Object& ret)
    {
        if (parse(s, ret)) {
            return;
        }
        else {
            COLOG_DEBUG(s);
            switch (m_p.peekType(s)) {
                case sss::json::Parser::kJE_STRING:
                    {
                        std::string str;
                        this->parse_string(s, str);
                        ret = varlisp::string_t(std::move(str));
                    }
                    break;

                case sss::json::Parser::kJE_NUMBER:
                    this->parse_number(s, ret);
                    break;

                case sss::json::Parser::kJE_TRUE:
                case sss::json::Parser::kJE_FALSE:
                    this->parse_bool(s, ret);
                    break;

                case sss::json::Parser::kJE_NULL:
                    this->parse_null(s, ret);
                    break;

                default:
                    SSS_POSITION_THROW(std::runtime_error,
                                       "unexpect string ", sss::raw_string(s));
                    break;
            }
        }
    }
    void skip_white_space(sss::string_view& s)
    {
        m_p.consumeWhiteSpace(s);
    }

    void parse_object(sss::string_view& s, Object& ret)
    {
        this->consume_or_throw(s, '{', "expect '{'");
        varlisp::Environment env;
        bool is_first = true;
        while (!s.empty() && s.front() != '}') {
            if (is_first) {
                is_first = false;
            }
            else {
                this->skip_white_space(s);
                this->consume_or_throw(s, ',', "expect ','");
            }
            std::string key;
            this->skip_white_space(s);
            this->parse_string(s, key);
            this->skip_white_space(s);
            this->consume_or_throw(s, ':', "expect ':'");
            Object val;
            this->skip_white_space(s);
            this->parse_value(s, val);
            env[key] = std::move(val);
            this->skip_white_space(s);
        }
        this->consume_or_throw(s, '}', "expect '}'");
        ret = std::move(env);
    }
    void parse_array(sss::string_view& s, Object& ret)
    {
        this->consume_or_throw(s, '[', "expect '['");
        varlisp::List list = varlisp::List::makeSQuoteList();
        auto back_it = varlisp::detail::list_back_inserter<varlisp::Object>(list);
        bool is_first = true;
        while (!s.empty() && s.front() != ']') {
            if (is_first) {
                is_first = false;
            }
            else {
                this->skip_white_space(s);
                this->consume_or_throw(s, ',', "expect ','");
            }
            Object val;
            this->skip_white_space(s);
            COLOG_DEBUG(s.front());
            this->parse_value(s, val);
            *back_it++ = val;
            this->skip_white_space(s);
        }
        this->consume_or_throw(s, ']', "expect ']'");
        ret = std::move(list);
    }

    void parse_string(sss::string_view& s, std::string& ret)
    {
        m_p.consumeString(s, ret);
    }

    void parse_number(sss::string_view& s, Object& ret)
    {
        std::tuple<double, int64_t> number;
        int index;
        if (!m_p.consumeNumber(s, number, index)) {
            return;
        }

        if (index == 0)
        {
            ret = std::get<0>(number);
        }
        else {
            ret = std::get<1>(number);
        }
    }

    void parse_bool(sss::string_view& s, Object& ret)
    {
        if (m_p.consumeTrue(s)) {
            ret = true;
        }
        else if (m_p.consumeFalse(s)) {
            ret = false;
        }
        else {
            SSS_POSITION_THROW(std::runtime_error,
                               "parse bool erro: ", s);
        }
    }

    void parse_null(sss::string_view& s, Object& ret)
    {
        if (m_p.consumeNull(s)) {
            ret = varlisp::Nill{};
        }
    }

    bool consume(sss::string_view& s, char c)
    {
        if (!s.empty() && s.front() == c) {
            s.pop_front();
            return true;
        }
        return false;
    }

    template<typename T>
    void consume_or_throw(sss::string_view& s, const T& v, sss::string_view msg)
    {
        if (!this->consume(s, v)) {
            if (s.empty()) {
                SSS_POSITION_THROW(std::runtime_error, "enconter: eof; ", msg.to_string());
            }
            else {
                SSS_POSITION_THROW(std::runtime_error, "enconter: ", sss::raw_char(s.front()), msg.to_string());
            }
        }
    }
};

Object parse(sss::string_view s)
{
    Object ret;
    JParser jp(s, ret);
    return ret;
}
} // namespace json
} // namespace varlisp
