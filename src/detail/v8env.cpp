// src/detail/v8env.cpp
#include "v8env.hpp"
#include "v8.h"


namespace varlisp::detail {

bool I_v8_value_visitor::visit( v8::Isolate* isolate, v8::Local<v8::Value> val )
{
    v8Env::Accepter acc(isolate);
    return acc.accept(this, val);
}

bool v8Env::Accepter::accept(I_v8_value_visitor * visitor, v8::Local<v8::Value> val) const
{
    bool matched = true;
    if (val->IsNull()) {
        visitor->visitNull(isolate);
    } else if (val->IsNumber()) {
        v8::Local<v8::Number> num = val.As<v8::Number>();
        visitor->visitNumber(isolate, num);
    } else if (val->IsBoolean()) {
        v8::Local<v8::Boolean> b = val.As<v8::Boolean>();
        visitor->visitBool(isolate, b);
    } else if (val->IsString()) {
        v8::Local<v8::String> str = val.As<v8::String>();
        visitor->visitString(isolate, str);
    } else if (val->IsArray()) {
        v8::Local<v8::Array> arr = val.As<v8::Array>();
        visitor->visitArray(isolate, arr);
    } else if (val->IsObject()) {
        v8::Local<v8::Object> obj = val.As<v8::Object>();
        visitor->visitObject(isolate, obj);
    } else {
        matched = false;
    }
    return matched;
}

std::string v8Env::Accepter::to_string(v8::Local<v8::Value> val) const
{
    v8::String::Utf8Value utf8(isolate, val);
    return *utf8;
}

std::string v8Env::Accepter::to_string(v8::Local<v8::String> str) const
{
    v8::String::Utf8Value utf8(isolate, str);
    return *utf8;
}

void v8Env::Accepter::loopArray(
    v8::Local<v8::Array> arr,
    const std::function<void(uint32_t index, v8::Local<v8::Value> val)>& func) const
{
    auto context = isolate->GetCurrentContext();
    for (uint32_t i = 0; i != arr->Length(); ++i) {
        v8::Local<v8::Value> val = arr->Get(context, i).ToLocalChecked();
        func(i, val);
    }
}

void v8Env::Accepter::loopObject(
    v8::Local<v8::Object> obj,
    const std::function<void(uint32_t index, v8::Local<v8::Value> key, v8::Local<v8::Value> val)>& func) const
{
    auto context = isolate->GetCurrentContext();
    // v8::Local<v8::Array> propertyNames = obj->GetPropertyNames(context).ToLocalChecked();
    v8::Local<v8::Array> props = obj->GetOwnPropertyNames(context).ToLocalChecked();

    for (uint32_t i = 0; i != props->Length(); ++i) {
        v8::Local<v8::Value> key = props->Get(context, i).ToLocalChecked();
        v8::Local<v8::Value> val = obj->Get(context, key).ToLocalChecked();
        func(i, key, val);
    }
}

// 创建一系列的双排发方法，即可模拟visitor；
//
// 应该不用额外，做wrapper，然后继承这样（wrapper的子类，各自继承不同的v8::Type）

std::string v8_value_type_name(v8::Isolate * isolate, const v8::Local<v8::Value>& value)
{
    v8::Local<v8::String> resTypeName = value->TypeOf(isolate);

    std::string typeName;
    typeName.resize(resTypeName->Utf8Length(isolate));

    (*resTypeName)->WriteUtf8(isolate, const_cast<char*>(typeName.data()));

    return typeName;
}

std::string v8_value_as_string(v8::Isolate * isolate, const v8::Local<v8::Value>& value)
{
    v8::String::Utf8Value utf8(isolate, value);
    return *utf8;
}

v8::Isolate* NewV8Isolate(const char * name,
                          std::unique_ptr<v8::Platform>& platform,
                          v8::Isolate::CreateParams& create_params)
{
    v8::V8::InitializeICUDefaultLocation(name);
    v8::V8::InitializeExternalStartupData(name);
    platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();

    //create_params = new v8::Isolate::CreateParams;
    create_params.array_buffer_allocator =
        v8::ArrayBuffer::Allocator::NewDefaultAllocator();

    return v8::Isolate::New(create_params);
}

v8Env::v8Env(const char * name)
    : isolate(NewV8Isolate(name, this->platform, this->create_params))
{
    //COLOG_INFO(isolate);
}

v8Env::~v8Env()
{
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
}

void v8Env::runWithResult(const std::string& raw_script, const std::function<void(v8::Local<v8::Value>)>& func)
{
    // NOTE isolate_scope 变量，以及下面的 context_scope，都要求在 stack 上创建——对于js的操作，必须被
    // 包裹在里面！
    v8::Isolate::Scope isolate_scope(isolate);

    // create a stack-allocated handle scope.
    v8::HandleScope handle_scope(isolate);

    v8::Local<v8::Context> context = v8::Context::New(isolate);
    // COLOG_DEBUG(SSS_VALUE_MSG(context.IsEmpty()));
    // COLOG_DEBUG(SSS_VALUE_MSG(context->IsContext()));

    v8::Context::Scope context_scope(context);

    v8::Local<v8::String> source =
        v8::String::NewFromUtf8(isolate, raw_script.c_str(),
                                v8::NewStringType::kNormal)
        .ToLocalChecked();

    v8::Local<v8::Script> script =
        v8::Script::Compile(context, source).ToLocalChecked();

    v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();

    if (func) {
        func(result);
    }
}

} // namespace varlisp::detail

