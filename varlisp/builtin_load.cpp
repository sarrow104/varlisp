#include "object.hpp"
#include "eval_visitor.hpp"
#include "interpreter.hpp"

#include <stdexcept>

#include <sss/raw_print.hpp>
#include <sss/colorlog.hpp>
#include <sss/path.hpp>

namespace varlisp {
// TODO
Object eval_load(varlisp::Environment& env, const varlisp::List& args)
{
    // NOTE ��Ȼ load���ڽ���������ôload���Ϳ��Գ������κεط�����������һ���ű���
    // �����ҵĽ����������ڽű���load�������ϣ������ڲ���Tokenizer����
    // push��popһ������Ĺ���ջ�������ǣ���ǰ��ֻ�ǣ�Tokenizerֻӵ����������ջ���൱��˫��������
    // ����Ƕ��load��ʹ�ã�����ܻ��������ģ�
    //
    // ���ǣ���ȫ�����������֣�
    //      1. ��Tokenizer���󣬳�Ϊ�����Ķ�ջ�Ķ���
    //      2. ����Ҫ����ʱ����Tokenizer�����Թ������á����൱������Tokenizer�Ķ�ջ��
    //
    // NOTE ���⣬��load��ʱ���Ƿ���Ҫ�½�һ��Environment�����أ�
    // ��������������⣻��������(load "path/to/script")�����ԣ��½�����
    // �ı�ʶ��(���󡢺����ȵ�)��Ӧ�÷ŵ��ĸ�Environment���أ�

#if 1
   Object path = boost::apply_visitor(eval_visitor(env), args.head);
   const std::string *p_path = boost::get<std::string>(&path);
   if (!p_path) {
       SSS_POSTION_THROW(std::runtime_error,
                         "read requies a path");
   }
   std::string full_path = sss::path::full_of_copy(*p_path);
   if (sss::path::file_exists(full_path) != sss::PATH_TO_FILE) {
       SSS_POSTION_THROW(std::runtime_error,
                         "path `", *p_path, "` not to file");
   }
   // varlisp::List content;
   // std::string line;
   std::string content;
   sss::path::file2string(full_path, content);

   varlisp::Interpreter * p_inter = env.getInterpreter();
   if (!p_inter) {
       SSS_POSTION_THROW(std::runtime_error, "env.getInterpreter return 0 ptr");
   }
   varlisp::Parser & parser = p_inter->get_parser();
   parser.parse(env, content, true);
   COLOG_INFO("(load ", sss::raw_string(*p_path), " complete)");
#endif
    return Object();
}


} // namespace varlisp
