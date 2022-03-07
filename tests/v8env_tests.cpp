#include <gtest/gtest.h>

#include "../src/detail/v8env.hpp"
#include "v8.h"

std::string v8_execute_result_as_string(varlisp::detail::v8Env& env, const std::string& scripts)
{
    std::string result;
    env.runWithResult(scripts, [&](v8::Local<v8::Value> val) {
        auto acc = env.makeAccepter();
        result = acc.to_string(val);
    });
    return result;
}

TEST(detail_v8env, basic)
{
    using namespace varlisp::detail;

    std::string content;
    sss::path::file2string("/Users/sarrow/ubuntu-home-back/project/Lisp/varlisp/src/crawler/samples/juejin.javascript.js", content);
    auto content1 = "var window = {}; " + content + "; window.__NUXT__.state.view.column.entry.article_info.status";
    auto content2 = "var window = {}; " + content + "; JSON.stringify(window.__NUXT__)";
    auto content3 = "var window = {}; " + content + "; var arr = []; with(window.__NUXT__.state.view.column.entry.article_info) { if (status == 2) { arr = [1, content]; } else if (status == 1) { arr = [2, mark_content]}}";

    v8Env env{"varlisp"};
    GTEST_ASSERT_EQ(v8_execute_result_as_string(env, "'hello world'"), "hello world");

    GTEST_ASSERT_EQ(v8_execute_result_as_string(env, content1), "2");

    GTEST_ASSERT_NE(v8_execute_result_as_string(env, content2), "[object Object]");
    GTEST_ASSERT_NE(v8_execute_result_as_string(env, content3), "TODO");

    GTEST_ASSERT_EQ(v8_execute_result_as_string(env, "[1,null,\"string\", ]"), "1,,string");
    //GTEST_ASSERT_EQ(v8_execute_result_as_string(env, "{num:1, v2:null, str:\"string\"}"), "TODO");
    GTEST_ASSERT_EQ(v8_execute_result_as_string(env, "{num:1}"), "1"); // field name or label
    GTEST_ASSERT_EQ(v8_execute_result_as_string(env, "var obj = {num:1, str: \"string\"}; JSON.stringify(obj) "), "{\"num\":1,\"str\":\"string\"}");

    // 如何同时返回多个值？
    //
    // 如果确实不能返回多个值，就需要保留环境（即，要么类似ECS，可以创建多个环境的能力；要么就整全局变量，只能运行一个js；然后多次run() 的形式，进行取值交互；
    //
    // 另外，多次获取比较深的值，可以使用with语句；例如：
    //
    // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Statements/with
    // ```
    // let a, x, y;
    // const r = 10;
    //
    // with (Math) {
    //   a = PI * r * r;
    //   x = r * cos(PI);
    //   y = r * sin(PI / 2);
    // }
    // ```
    //
    // 仅针对 问题的话
}

