#include <array>

#include <libplatform/libplatform.h>
#include <v8.h>

#include <v8pp/ptr_traits.hpp>
#include <v8pp/convert.hpp>

#include <sss/debug/value_msg.hpp>
#include <sss/colorlog.hpp>
#include <sss/pretytypename.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/list_iterator.hpp"
#include "../detail/car.hpp"
#include "../detail/v8env.hpp"

namespace varlisp {

struct varlisp_v8_visitor : varlisp::detail::I_v8_value_visitor {
    typedef varlisp::detail::I_v8_value_visitor base_type;
    bool visit(v8::Isolate *isolate, v8::Local<v8::Value> val) override;
    void visitNull(v8::Isolate *isolate) override;
    void visitArray(v8::Isolate *isolate, v8::Local<v8::Array> val) override;
    void visitNumber(v8::Isolate *isolate, v8::Local<v8::Number> val) override;
    void visitBool(v8::Isolate *isolate, v8::Local<v8::Boolean> val) override;
    void visitObject(v8::Isolate *isolate, v8::Local<v8::Object> val) override;
    void visitString(v8::Isolate *isolate, v8::Local<v8::String> val) override;

    varlisp::Object obj;
};

REGIST_BUILTIN("v8-run", 1, 1, eval_v8_run,
               "; embeded v8 vm run script\n"
               "(v8-run \"js script\") -> varlisp-obj");

/**
 * @brief (v8-run "js script") -> varlisp-obj
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_v8_run(varlisp::Environment &env, const varlisp::List &args)
{
    static varlisp::detail::v8Env v8env{"varlisp"};

    const char * funcName = "v8-run";
    std::array<Object, 1> objs;

    const string_t* p_js_script =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    varlisp_v8_visitor myVisitor;
    v8env.runWithResult(*p_js_script->gen_shared(), [&](v8::Local<v8::Value> val) {
        auto acc = v8env.makeAccepter();
        acc.accept(&myVisitor, val);
    });

    return myVisitor.obj;
}

bool varlisp_v8_visitor::visit(v8::Isolate *isolate, v8::Local<v8::Value> val)
{
    if (!base_type::visit(isolate, val)) {
        varlisp::detail::v8Env::Accepter acc{isolate};
        this->obj = varlisp::string_t{acc.to_string(val)};
    }
    return true;
}

void varlisp_v8_visitor::visitNull(v8::Isolate *isolate)
{
    this->obj = varlisp::Nill{};
}

void varlisp_v8_visitor::visitArray(v8::Isolate *isolate, v8::Local<v8::Array> val)
{
    this->obj = varlisp::List::makeSQuoteList();
    auto m_list = boost::get<varlisp::List>(&this->obj)->get_slist();

    varlisp::detail::v8Env::Accepter acc{isolate};
    acc.loopArray(val, [&](uint32_t i, v8::Local<v8::Value> val) {
        (void)i;
        varlisp_v8_visitor innerVisitor;
        innerVisitor.visit(isolate, val);
        m_list->append(std::move(innerVisitor.obj));
    });
}

void varlisp_v8_visitor::visitNumber(v8::Isolate *isolate, v8::Local<v8::Number> val)
{
    // TODO FIXME distinguish int64_t and double
    this->obj = v8pp::from_v8<int64_t>(isolate, val);
}

void varlisp_v8_visitor::visitBool(v8::Isolate *isolate, v8::Local<v8::Boolean> val)
{
    this->obj = v8pp::from_v8<bool>(isolate, val);
}

void varlisp_v8_visitor::visitObject(v8::Isolate *isolate, v8::Local<v8::Object> val)
{
    this->obj = varlisp::Environment();
    auto p_env = boost::get<varlisp::Environment>(&this->obj);

    varlisp::detail::v8Env::Accepter acc{isolate};
    acc.loopObject(val, [&](uint32_t i, v8::Local<v8::Value> key, v8::Local<v8::Value> val) {
        (void)i;
        auto keyName = acc.to_string(key);
        varlisp_v8_visitor valVisitor;
        valVisitor.visit(isolate, val);

        p_env->operator[](keyName) = std::move(valVisitor.obj);
    });
}

void varlisp_v8_visitor::visitString(v8::Isolate *isolate, v8::Local<v8::String> val)
{
    varlisp::detail::v8Env::Accepter acc{isolate};

    this->obj = varlisp::string_t{acc.to_string(val)};
}

}  // namespace varlisp
