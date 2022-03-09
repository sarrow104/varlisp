// src/detail/v8env.hpp
#pragma once

#include <libplatform/libplatform.h>
#include <v8.h>

#include <memory>
#include <string>

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/path.hpp>

namespace varlisp::detail {

struct I_v8_value_visitor
{
    I_v8_value_visitor() = default;
    I_v8_value_visitor(const I_v8_value_visitor&) = default;
    I_v8_value_visitor(I_v8_value_visitor&&) = default;
    I_v8_value_visitor& operator=(const I_v8_value_visitor&) = default;
    I_v8_value_visitor& operator=(I_v8_value_visitor&&) = default;
    //I_v8_value_visitor() = default;
    virtual ~I_v8_value_visitor() = default;

    virtual bool visit( v8::Isolate* isolate, v8::Local<v8::Value> val );

    virtual void visitNull( v8::Isolate* isolate ) = 0;

    virtual void visitNumber( v8::Isolate* isolate, v8::Local<v8::Number> val ) = 0;
    virtual void visitBool(   v8::Isolate* isolate, v8::Local<v8::Boolean> val ) = 0;
    virtual void visitString( v8::Isolate* isolate, v8::Local<v8::String> val ) = 0;
    virtual void visitArray(  v8::Isolate* isolate, v8::Local<v8::Array>  val ) = 0;
    virtual void visitObject( v8::Isolate* isolate, v8::Local<v8::Object> val ) = 0;
};


class v8Env {
public:
    explicit v8Env(const char * name);
    v8Env(v8Env&&) = default;
    v8Env& operator=(v8Env&&) = default;

    v8Env(const v8Env&) = delete;
    v8Env& operator=(const v8Env&) = delete;

    ~v8Env();

    void runWithResult(const std::string& raw_script, const std::function<void(v8::Local<v8::Value>)>& func);

    struct Accepter
    {
    private:
        v8::Isolate* isolate;

    public:
        explicit Accepter(v8::Isolate* isolate)
            : isolate( isolate ) {}
        Accepter(const Accepter&) = default;
        Accepter(Accepter&&) = default;

        Accepter& operator=(const Accepter&) = default;
        Accepter& operator=(Accepter&&) = default;
        ~Accepter() = default;

        bool accept(I_v8_value_visitor * visitor, v8::Local<v8::Value> val) const;

        std::string to_string(v8::Local<v8::Value> val) const;
        std::string to_string(v8::Local<v8::String> str) const;

        void loopArray(v8::Local<v8::Array> arr, const std::function<void(uint32_t index, v8::Local<v8::Value> val)>& func) const;
        void loopObject(v8::Local<v8::Object> obj, const std::function<void(uint32_t index, v8::Local<v8::Value> key, v8::Local<v8::Value> val)>& func) const;
    };

    Accepter makeAccepter() const
    {
        return Accepter{isolate};
    }

private:
    std::unique_ptr<v8::Platform> platform;
    v8::Isolate::CreateParams create_params;
    v8::Isolate* isolate;
};

} // namespace varlisp::detail
