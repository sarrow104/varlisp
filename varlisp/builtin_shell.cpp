#include "object.hpp"
#include "eval_visitor.hpp"

#include <sss/path.hpp>
#include <sss/path/glob_path.hpp>
#include <sss/colorlog.hpp>
#include <sss/raw_print.hpp>

namespace varlisp {
// Ӧ����δ���shell-evalʱ��Ĳ�����
// ���磬�û������ṩһ���������������ַ�����������ִ�е�����Լ�������
// ��ʱ�������������أ��������ţ�Ȼ��ִ�У����군�ˡ�
//
// ͬʱ��Ҳ�п�����Ҫִ�е�����Լ��ⲿ�����Ĳ������Ƿֿ��ġ����磬
// ����һ���������Ǵ���·�����ַ��������һ��пո���ô��������ţ�
// ���߽���ת�壬����һ������Ķ����ˡ�
//
// ����Ȼ���������ֲ����ݵĹ�����ʽ��
Object eval_shell(varlisp::Environment& env, const varlisp::List& args)
{
    // TODO
    return Object{};
}
Object eval_cd(varlisp::Environment& env, const varlisp::List& args)
{
    Object target_path = boost::apply_visitor(eval_visitor(env), args.head);
    const std::string *p_path = boost::get<std::string>(&target_path);
    if (!p_path) {
        SSS_POSTION_THROW(std::runtime_error,
                          "shell-cd: requie one path string!");
    }
    sss::path::chgcwd(*p_path);
    COLOG_INFO("(shell-cd: ", sss::raw_string(*p_path), " complete)");
    return Object(sss::path::getcwd());
}
// {"ls",          0, -1,  &eval_ls},      // ����������������Ϊ·�����ַ�����Ϊ������ö�ٳ�����·��
Object eval_ls(varlisp::Environment& env, const varlisp::List& args)
{
    varlisp::List ret;
    const List * p = &args;
    List * p_list = &ret;
    if (args.length()) {
        while (p && p->head.which()) {
            Object ls_arg = boost::apply_visitor(eval_visitor(env), p->head);
            const std::string *p_ls_arg = boost::get<std::string>(&ls_arg);
            if (!p_ls_arg) {
                SSS_POSTION_THROW(std::runtime_error, "shell-ls: require string-type args");
            }
            switch (sss::path::file_exists(*p_ls_arg)) {
                case sss::PATH_TO_FILE:
                    p_list = p_list->next_slot();
                    p_list->head = *p_ls_arg;
                    break;

                case sss::PATH_TO_DIRECTORY:
                    {
                        sss::path::file_descriptor fd;
                        sss::path::glob_path gp(*p_ls_arg, fd);
                        while (gp.fetch()) {
                            if (fd.is_normal_dir()) {
                                p_list = p_list->next_slot();
                                p_list->head = std::string(fd.get_name()) + sss::path::sp_char;
                            }
                            else if (fd.is_normal_file()) {
                                p_list = p_list->next_slot();
                                p_list->head = std::string(fd.get_name());
                            }
                        }
                    }
                    break;

                case sss::PATH_NOT_EXIST:
                    COLOG_ERROR("(shell-ls: path", sss::raw_string(*p_ls_arg), "not exists)");
                    break;
            }
            if (p->tail.empty()) {
                p = 0;
            }
            else {
                p = &p->tail[0];
            }
        }
    }
    else {
        sss::path::file_descriptor fd;
        sss::path::glob_path gp(".", fd);
        while (gp.fetch()) {
            if (fd.is_normal_dir()) {
                p_list = p_list->next_slot();
                p_list->head = std::string(fd.get_name()) + sss::path::sp_char;
            }
            else if (fd.is_normal_file()) {
                p_list = p_list->next_slot();
                p_list->head = std::string(fd.get_name());
            }
        }
    }
    return Object(ret);
}
Object eval_pwd(varlisp::Environment& env, const varlisp::List& args)
{
    (void)env;
    (void)args;
    return Object(sss::path::getcwd());
}

} // namespace varlisp
