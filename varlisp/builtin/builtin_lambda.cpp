#include <vector>
#include <cstring>

#include <re2/re2.h>

#include "../object.hpp"

#include "../builtin_helper.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../json_print_visitor.hpp"
#include "../json/parser.hpp"
#include "../detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("apply", 2, 2, eval_apply,
               "; apply 将函数作用于参数列表上，并返回结果\n"
               "(apply func '(args...))");

Object eval_apply(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "apply";
    std::array<Object, 1> objs;
    const auto * p_list = getQuotedList(env, args.nth(1), objs[0]);
    varlisp::requireOnFaild<varlisp::QuoteList>(p_list, funcName, 1, DEBUG_INFO);

    return varlisp::apply(env, args.nth(0), *p_list);
}

REGIST_BUILTIN("curry", 1, -1, eval_curry,
               "; curry 科里化\n"
               "; var... 这些参数的求值时机是？\n"
               "(curry func var...) -> (lambda ($1) (apply func '($1 var...)))");

// /home/sarrow/project/node.js/curry/test.js:17
// http://blog.csdn.net/jason5186/article/details/43764331
//
// 上面两个例子，都是可以讲参数的赋值过程给拆分开。如果参数需要在不同的
// 程序运行状态下确定，这样的curry化，确实是一个好工具。
// 这就相当于不断地curry。
//
// 我可以模拟这个吗？
// 可以的。这里需要判断func所需要的最小参数。
// 如果不足，就返回(curry func
// (define (curry f . args)
//   (lambda (x)
//     (apply f (append args
//                      (list x)))))
Object eval_curry(varlisp::Environment& env, const varlisp::List& args)
{
    std::vector<std::string> lambda_args;
    char buffer[32] = {0};
    std::sprintf(buffer, "$_%p", &args.nth(0));
    std::string var_name = buffer;
    lambda_args.emplace_back(var_name);
    varlisp::string_t       lambda_doc;
    std::vector<varlisp::Object> lambda_body;
    
    auto true_args = varlisp::List();
    true_args.append(varlisp::symbol(var_name));
    true_args.append_list(args.tail());

    lambda_body.emplace_back(varlisp::List{
        varlisp::symbol("apply"),
            args.nth(0),
            varlisp::List::makeSQuoteObj(true_args)
    });
    return varlisp::Lambda(std::move(lambda_args), std::move(lambda_doc),
                           std::move(lambda_body));
}

REGIST_BUILTIN("partial", 1, -1, eval_partial,
               "; partial binding\n"
               "; var... 这些参数的求值时机是？\n"
               "(partial func var... $1...) -> (lambda ($1...) (apply f '(var... $...)))");

Object eval_partial(varlisp::Environment& env, const varlisp::List& args)
{
    
    static RE2 re("\\$\\d+");

    // 1. std::map<int, symbol> id-名字；
    std::map<int, std::string> arg_map;
    auto true_args = varlisp::List();
    for (size_t i = 1; i < args.length(); ++i) {
        if (auto * p_val = boost::get<varlisp::symbol>(&args.nth(i))) {
            if (RE2::FullMatch(p_val->name(), re)) {
                auto p_id_str = p_val->name().c_str() + 1;
                int id = std::stoi(p_id_str);
                // NOTE id可重复使用；
                if (arg_map.find(id) == arg_map.end()) {
                    char buffer[32] = {0};
                    std::sprintf(buffer, "$_%d_%p", id, p_id_str);
                    arg_map[id] = buffer;
                }
                true_args.append(varlisp::symbol(arg_map[id]));
            }
            else {
                true_args.append(args.nth(i));
            }
        }
        else {
            true_args.append(args.nth(i));
        }
    }

    // 2. 将map中形参，按stoi后的序号，生成形参列表--std::vecor<string>
    std::vector<std::string> lambda_args;
    for (auto item : arg_map) {
        lambda_args.push_back(item.second);
    }

    // 3. 同curry的apply构造方式，生成lambda，并返回；
    varlisp::string_t       lambda_doc;
    std::vector<varlisp::Object> lambda_body;
    lambda_body.emplace_back(varlisp::List{
        varlisp::symbol("apply"),
            args.nth(0),
            varlisp::List::makeSQuoteObj(true_args)
    });
    return varlisp::Lambda(std::move(lambda_args), std::move(lambda_doc),
                           std::move(lambda_body));
}
} // namespace varlisp
