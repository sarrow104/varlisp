// src/detail/v8env.hpp
#pragma once

#include <libplatform/libplatform.h>
#include <v8.h>

#include <string>
#include <memory>

#include <cstdio>
#include <cstdlib>

#include <iostream>

#include <sss/path.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/colorlog.hpp>

namespace varlisp {
namespace detail {

struct I_v8_value_visitor
{
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

    ~v8Env();

    void runWithResult(std::string raw_script, std::function<void(v8::Local<v8::Value>)> func);

    struct Accepter
    {
        explicit Accepter(v8::Isolate* isolate)
            : isolate( isolate ) {}
        ~Accepter() {}

        v8::Isolate* isolate;

        bool accept(I_v8_value_visitor * visitor, v8::Local<v8::Value> val) const;

        std::string to_string(v8::Local<v8::Value> val) const;
        std::string to_string(v8::Local<v8::String> str) const;

        void loopArray(v8::Local<v8::Array> arr, std::function<void(uint32_t index, v8::Local<v8::Value> val)> func) const;
        void loopObject(v8::Local<v8::Object> obj, std::function<void(uint32_t index, v8::Local<v8::Value> key, v8::Local<v8::Value> val)> func) const;
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

} // namespace detail
} // namespace varlisp


//int main (int argc, char *argv[])
//{
//    (void)argc;
//
//    std::string content;
//    sss::path::file2string("/Users/sarrow/ubuntu-home-back/project/Lisp/varlisp/src/crawler/samples/juejin.javascript.js", content);
//    auto content1 = "var window = {}; " + content + "; window.__NUXT__.state.view.column.entry.article_info.status";
//    auto content2 = "var window = {}; " + content + "; window.__NUXT__";
//    auto content3 = "var window = {}; " + content + "; var arr = []; with(window.__NUXT__.state.view.column.entry.article_info) { if (status == 2) { arr = [1, content]; } else if (status == 1) { arr = [2, mark_content]}}";
//
//    if (true) {
//        v8Env env{argv[0]};
//        COLOG_INFO(env.run("'hello world'"));
//        COLOG_INFO(env.run(content1));
//        COLOG_INFO(env.run(content2));
//        COLOG_INFO(env.run(content3));
//        COLOG_INFO(env.run("[1,null,\"string\", ]"));
//        //COLOG_INFO(env.run("{num:1, v2: null, str: \"string\"}"));
//        COLOG_INFO(env.run("var obj = {num:1, str: \"string\"}; obj "));
//    } else {
//        auto res = rawRun(argv[0], content);
//        std::cout << res << std::endl;
//
//        //res = rawRun(argv[0], "hello world");
//        //std::cout << res << std::endl;
//    }
//
//    // 如何同时返回多个值？
//    //
//    // 如果确实不能返回多个值，就需要保留环境（即，要么类似ECS，可以创建多个环境的能力；要么就整全局变量，只能运行一个js；然后多次run() 的形式，进行取值交互；
//    //
//    // 另外，多次获取比较深的值，可以使用with语句；例如：
//    //
//    // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/with
//    // ```
//    // let a, x, y;
//    // const r = 10;
//    //
//    // with (Math) {
//    //   a = PI * r * r;
//    //   x = r * cos(PI);
//    //   y = r * sin(PI / 2);
//    // }
//    // ```
//    //
//    // 仅针对 问题的话
//
//    return EXIT_SUCCESS;
//}
