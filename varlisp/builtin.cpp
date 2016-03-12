#include <sss/util/PostionThrow.hpp>
#include <sss/log.hpp>

#include <cmath>

#include "builtin.hpp"
#include "cast2double_visitor.hpp"
#include "eval_visitor.hpp"
#include "print_visitor.hpp"
#include "environment.hpp"
#include "strict_equal_visitor.hpp"
#include "strict_less_visitor.hpp"

namespace varlisp {

    typedef Object (*eval_func_t)(varlisp::Environment& env, const varlisp::List& args);

    Object eval_add(varlisp::Environment& env, const varlisp::List& args)
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        double sum = 0;
        const List * p = &args;
        while (p && p->head.which()) {
            sum += boost::apply_visitor(cast2double_visitor(env), p->head);
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, sum);
        return Object(sum);
    }

    Object eval_sub(varlisp::Environment& env, const varlisp::List& args)
    {
        int args_cnt = args.length();
        if (args_cnt == 1) {
            return Object(-boost::apply_visitor(cast2double_visitor(env),
                                                args.head));
        }
        else {
            double sum = boost::apply_visitor(cast2double_visitor(env), args.head);
            const List * p = &args.tail[0];
            while (p && p->head.which()) {
                sum -= boost::apply_visitor(cast2double_visitor(env), p->head);
                if (p->tail.empty()) {
                    p = 0;
                }
                else {
                    p = &p->tail[0];
                }
            }
            return Object(sum);
        }
    }

    Object eval_mul(varlisp::Environment& env, const varlisp::List& args)
    {
        double mul = boost::apply_visitor(cast2double_visitor(env), args.head);
        std::cout << "args.head " << args.head.which() << " ";
        boost::apply_visitor(print_visitor(std::cout), args.head);
        std::cout << std::endl;
        if (args.head.which() == 5) {
            const Object& ref = env[boost::get<varlisp::symbol>(args.head).m_data];
            std::cout << " ";
            boost::apply_visitor(print_visitor(std::cout), ref);
            std::cout << std::endl;
        }
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, mul);
        const List * p = &args.tail[0];
        while (mul && p && p->head.which()) {
            mul *= boost::apply_visitor(cast2double_visitor(env), p->head);
            SSS_LOG_EXPRESSION(sss::log::log_DEBUG, mul);
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
        return Object(mul);
    }

    Object eval_div(varlisp::Environment& env, const varlisp::List& args)
    {
        double mul = boost::apply_visitor(cast2double_visitor(env), args.head);
        const List * p = &args.tail[0];
        while (mul && p && p->head.which()) {
            double div = boost::apply_visitor(cast2double_visitor(env), p->head);
            if (!div) {
                throw std::runtime_error(" divide by zero!");
            }
            mul /= div;
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
        return Object(mul);
    }

    Object eval_pow(varlisp::Environment& env, const varlisp::List& args)
    {
        double lhs = boost::apply_visitor(cast2double_visitor(env), args.head);
        double rhs = boost::apply_visitor(cast2double_visitor(env), args.tail[0].head);

        return Object(std::pow(lhs, rhs));
    }

    // ����drracket��˵���Ƚ������������ֱ��Ҫ��ת����real��
    // ���ʧ�ܣ����׳��쳣��
    // ����lambda��ʽ����1.2ʵ���ıȽϴ�С��������׳��쳣��
    // > (if (> fib 1) 1 2)
    // >: contract violation
    //   expected: real?
    //   given: #<procedure:fib>
    //   argument position: 1st
    //   other arguments...:
    //    1
    // ���У�
    // > (define fib (lambda (x) (if (> x 2) (+ (fib (- x 1)) (fib (- x 2))) 1)))
    Object eval_eq(varlisp::Environment& env, const varlisp::List& args)
    {
        return Object(boost::apply_visitor(strict_equal_visitor(),
                                           args.head,
                                           args.tail[0].head));
    }

    Object eval_gt(varlisp::Environment& env, const varlisp::List& args)
    {
        return Object(
            !boost::apply_visitor(strict_equal_visitor(),
                                  args.tail[0].head,
                                  args.head) &&
            boost::apply_visitor(strict_less_visitor(env),
                                 args.tail[0].head,
                                 args.head));
    }

    Object eval_lt(varlisp::Environment& env, const varlisp::List& args)
    {
        return Object(boost::apply_visitor(strict_less_visitor(env),
                                           args.head,
                                           args.tail[0].head));
    }

    Object eval_ge(varlisp::Environment& env, const varlisp::List& args)
    {
        return Object(boost::apply_visitor(strict_less_visitor(env),
                                           args.tail[0].head,
                                           args.head));
    }

    Object eval_le(varlisp::Environment& env, const varlisp::List& args)
    {
        return Object(
            boost::apply_visitor(strict_equal_visitor(),
                                 args.tail[0].head,
                                 args.head)
            ||
            boost::apply_visitor(strict_less_visitor(env),
                                 args.head,
                                 args.tail[0].head));

    }

    Object eval_eval(varlisp::Environment& env, const varlisp::List& args)
    {
        std::cout << __func__ << " ";
        boost::apply_visitor(print_visitor(std::cout), args.head);
        std::cout << std::endl;
        int arg_length = args.length();
        if (arg_length == 2) {
            SSS_POSTION_THROW(std::runtime_error,
                             "eval only support one arguments now!");
            // TODO
            // ���ڻ������������Ը��ݴ��������symbol��Ȼ��ͨ��ȫ�ֺ�������ȡ��
            // �����������ã�Ȼ��Ӧ�õ����
        }
        // ��Ҫ�ȶԲ���evalһ�Σ�Ȼ����evalһ�Σ�
        // Ϊʲô��Ҫ���Σ�
        // ��Ϊ����Ȼ��ͨ��eval���ý����ģ���ô�����eval������lisp���﷨������
        // �Ƚ���()�ڲ���Ȼ����Ϊ�ⲿ�Ĳ����ٴν�����
        //
        // ���ԣ�������eval�������ֱ���ԭ�е��û������������eval�ؼ����Լ������Ķ�����
        varlisp::Object first_res = boost::apply_visitor(eval_visitor(env), args.head);
        return boost::apply_visitor(eval_visitor(env), first_res);
    }

    // ������ʽ��
    // �����䣻-1��ʾ����
    struct builtin_info_t {
        const char *    name;
        int             min;
        int             max;
        eval_func_t     eval_fun;
    };

    const builtin_info_t builtin_infos[]
        = {
            {"+",       1, -1, &eval_add},
            {"-",       1, -1, &eval_sub},
            {"*",       2, -1, &eval_mul},
            {"/",       2, -1, &eval_div},
            {"^",       2,  2, &eval_pow},
            {"=",       2,  2, &eval_eq},
            {">",       2,  2, &eval_gt},
            {"<",       2,  2, &eval_lt},
            {">=",      2,  2, &eval_ge},
            {"<=",      2,  2, &eval_le},
            {"eval",    1,  2, &eval_eval},
        };

    void Builtin::print(std::ostream& o) const
    {
        o << "#<builtin:" << builtin_infos[this->m_type].name << ">";
    }

    Builtin::Builtin(type_t type)
        : m_type(type)
    {
    }

    Object Builtin::eval(varlisp::Environment& env, const varlisp::List& args) const
    {
        SSS_LOG_FUNC_TRACE(sss::log::log_DEBUG);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, args);
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, builtin_infos[m_type].name);
        int arg_length = args.length();
        int arg_min = builtin_infos[m_type].min;
        int arg_max = builtin_infos[m_type].max;
        if (arg_min > 0 && arg_length < arg_min) {
            SSS_POSTION_THROW(std::runtime_error,
                              " need at least " << arg_min << " parameters. "
                              << " but provided " << arg_length);
        }
        if (arg_max > 0 && arg_length > arg_max) {
            SSS_POSTION_THROW(std::runtime_error,
                              " need at most " << arg_max << " parameters."
                              << " but provided " << arg_length);
        }
        return builtin_infos[m_type].eval_fun(env, args);
    }

    void Builtin::regist_builtin_function(Environment& env)
    {
        env["+"] = varlisp::Builtin(varlisp::Builtin::TYPE_ADD);
        env["-"] = varlisp::Builtin(varlisp::Builtin::TYPE_SUB);
        env["*"] = varlisp::Builtin(varlisp::Builtin::TYPE_MUL);
        env["/"] = varlisp::Builtin(varlisp::Builtin::TYPE_DIV);

        env["^"] = varlisp::Builtin(varlisp::Builtin::TYPE_POW);

        env["="] = varlisp::Builtin(varlisp::Builtin::TYPE_EQ);

        env[">"] = varlisp::Builtin(varlisp::Builtin::TYPE_GT);
        env["<"] = varlisp::Builtin(varlisp::Builtin::TYPE_LT);
        env[">="] = varlisp::Builtin(varlisp::Builtin::TYPE_GE);
        env["<="] = varlisp::Builtin(varlisp::Builtin::TYPE_LE);

        env["eval"] = varlisp::Builtin(varlisp::Builtin::TYPE_EVAL);
    }
} // namespace varlisp
