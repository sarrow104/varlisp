#include "object.hpp"
#include "eval_visitor.hpp"

namespace varlisp {
Object eval_eval(varlisp::Environment& env, const varlisp::List& args)
{
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

} // namespace varlisp
