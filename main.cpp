#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <initializer_list>

#include <linenoise.hpp>

#include <sss/debug/value_msg.hpp>
#include <sss/algorithm.hpp>
#include <sss/colorlog.hpp>
#include <sss/log.hpp>
#include <sss/path.hpp>
#include <sss/utlstring.hpp>
#include <sss/string_view.hpp>
#include <sss/utlstring.hpp>
#include <sss/Terminal.hpp>
#include <sss/CMLParser.hpp>

#include "src/interpreter.hpp"
#include "src/tokenizer.hpp"
#include "src/String.hpp"

//http://stackoverflow.com/questions/6364681/how-to-handle-control-c-in-a-boost-tcp-udp-server
#include <signal.h> // or <csignal> in C++

volatile int WE_MUST_STOP = 0;
void ctrlchandler(int v) {
    std::cout << __PRETTY_FUNCTION__ << v << std::endl;
    /*...*/ WE_MUST_STOP = 1;
}
void killhandler(int v) {
    std::cout << __PRETTY_FUNCTION__ << v << std::endl;
    /*...*/ WE_MUST_STOP = 1;
    /*...*/ WE_MUST_STOP = 2;
}

//
// int main() {
//   signal(SIGINT, ctrlchandler);
//   signal(SIGTERM, killhandler);
//   /* ... */
//
//   // e.g. main loop like this:
//   while(pump_loop() && 0 == WE_MUST_STOP) { }
//
// }
//
// http://www.boost.org/doc/libs/1_47_0/doc/html/boost_asio/overview/signals.html

// TODO FIXME
// 这里需要和tokenizer同步修改！
// 功能应该独立出来！
const char* find_identifier(const char* buf)
{
    if (!buf || !buf[0]) {
        return 0;
    }
    const char* ret = std::strchr(buf, '\0');
    if (std::isspace(*(ret - 1))) {
        return 0;
    }
    while ((std::distance(buf, ret - 1) >= 0 &&
           (std::isalnum(*(ret - 1)) || *(ret - 1) == '-'))
           ||
           *(ret - 1) == '_') {
        ret--;
    }
    return ret;
}

int get_indent(const std::string& line)
{
    int indent = 0;
    while (indent < int(line.length()) && std::isblank(line[indent])) {
        indent++;
    }
    return indent;
}

int test_other()
{
    std::cout << sss::raw_string("\\");
    return EXIT_SUCCESS;
}

int test_construct()
{
    varlisp::Interpreter& interpreter = varlisp::Interpreter::get_instance();
    // interpreter.eval("(define i 0)");
    interpreter.eval(
        "(define (sqrt-iter guess x) (if (good-enough guess x) guess "
        "(sqrt-iter (improve guess x) x)))");
    interpreter.eval("(define (improve guess x) (average guess (/ x guess)))");
    interpreter.eval("(define (average x y) (/ (+ x y) 2))");
    interpreter.eval(
        "(define (good-enough guess x) (< (abs (- (square guess) x)) 0.001))");
    interpreter.eval("(define (sqrt x) (sqrt-iter 1.0 x))");
    interpreter.eval("(define (square x) (* x x))");
    interpreter.eval("(define (abs x) (if (< x 0) (- x) x))");
    interpreter.eval("(sqrt 9)");

    // interpreter.eval("(define fibonacci (lambda (n) (define iter (lambda (i
    // n1 n2) (if (= i 0) n2 (iter (- i 1) n2 (+ n1 n2))))) (iter n 0 1)))");
    // interpreter.eval("(fibonacci 10)");
    // interpreter.eval("(define x 3)");
    // interpreter.eval("(- x 1)");
    // interpreter.eval("(> x 2)");
    // interpreter.eval("(fib (- x 1))");
    // interpreter.eval("(fib 5)");
    // interpreter.eval("(fib 2)");
    // interpreter.eval("(fib 3)");
    return EXIT_SUCCESS;
}

const char * gen_prompt_continue(varlisp::Interpreter& i)
{
    auto pt = i.get_parser().get_parenthese_stack();
    static char buf[64] = "";

    std::sprintf(buf, "%s(%d[%d{%d: %s", sss::Terminal::dark.data(),
                 std::get<0>(pt), std::get<1>(pt), std::get<2>(pt),
                 sss::Terminal::end.data());
    return buf;
}

int Interpret(bool echo_in_load, bool quit_on_load_complete,
              bool load_init_script, int argc, char* argv[])
{
    varlisp::Interpreter& interpreter = varlisp::Interpreter::get_instance();

    if (load_init_script) {
        std::string preload_script = sss::path::dirname(sss::path::getbin());
        sss::path::append(preload_script, "init.varlisp");
        if (sss::path::filereadable(preload_script)) {
            interpreter.load(preload_script, echo_in_load);
        }
    }
    for (int i = 0; i < argc && interpreter.is_status(varlisp::Interpreter::status_OK); ++i) {
        std::string script_path = sss::path::full_of_copy(argv[i]);
        if (!sss::path::filereadable(script_path)) {
            SSS_POSITION_THROW(std::runtime_error,
                              "(path", sss::raw_string(argv[i]), " not readable)");
        }
        interpreter.load(script_path, echo_in_load);
    }
    if (quit_on_load_complete) {
        return EXIT_SUCCESS;
    }
    if (!interpreter.is_status(varlisp::Interpreter::status_OK)) {
        if (interpreter.is_status(varlisp::Interpreter::status_ERROR)) {
            SSS_POSITION_THROW(std::runtime_error,
                              "parseError");
        }
        return EXIT_SUCCESS;
    }

    linenoise::SetHistoryMaxLen(100);

    // NOTE linenoise默认是单行模式；此时，大段的输入，会导致响应变慢。
    // 因此，修改为多行模式
    linenoise::SetMultiLine(true);
    // TODO NOTE 为linenoise::SetCompletionCallback回调函数，增加一个触发位置的参数。
    // 以便完成在中间补全。
    // 不然，现有的补全，如果在行中间触发，会定位到行末尾；
    // 为了正常补全，只能提前回车，用多行。
    // 但这样，就需要去数左括号次数，然后添加右括号。
    linenoise::SetCompletionCallback([&interpreter](
        const char* editBuffer, std::vector<std::string>& completions) {

        const char* last_identifier = find_identifier(editBuffer);
        if (!last_identifier) {
            int width = strlen(editBuffer);
            int tabsize = 8;
            // width = width - width % tabsize + tabsize;
            completions.push_back(editBuffer +
                                  std::string(tabsize - width % tabsize, ' '));
            return;
        }
        std::string prefix =
            std::string(editBuffer, std::distance(editBuffer, last_identifier));
        interpreter.retrieve_symbols(completions, last_identifier);
        for (auto& symbol : completions) {
            symbol = prefix + symbol + " ";
        }
        if (sss::is_begin_with("quit", last_identifier)) {
            completions.push_back(prefix + "quit) ");
        }
    });

    std::string hist_path = sss::path::dirname(sss::path::getbin());
    sss::path::append(hist_path, "history.txt");

    std::cout << SSS_VALUE_MSG(hist_path) << std::endl;

    linenoise::LoadHistory(hist_path.c_str());
    std::string app = sss::path::basename(sss::path::getbin());
    varlisp::Interpreter::status_t st = varlisp::Interpreter::status_OK;

    std::string last_command;
    int indent = 0;

    std::string prompt_start = sss::Terminal::dark.data() + std::string("> ") + sss::Terminal::end.data();
    // std::string prompt_continue = sss::Terminal::dark.data() + std::string(": ") + sss::Terminal::end.data();
    while (st != varlisp::Interpreter::status_ERROR &&
           st != varlisp::Interpreter::status_QUIT) {
        // NOTE 缩进保持功能丢失了。
        // 之前，我修改了，install 到 ~/extra/linenoise/ 下的 linenoise.hpp
        // ，以支持 indent风格。
        // 但是，我在 pull
        // upstream，对linenoise的更新之后，忘记我对哪里，做过修改……
        // 不过，好消息是，新的linenoise，对于tab补全遗留问题，做了修改。
        auto line = linenoise::Readline(
            st == varlisp::Interpreter::status_UNFINISHED
                ? gen_prompt_continue(interpreter) : prompt_start.c_str(), indent);
        indent = get_indent(line);

        try {
            switch (st) {
            case varlisp::Interpreter::status_UNFINISHED:
                st = interpreter.eval(line);
                break;

            case varlisp::Interpreter::status_OK:
                if (sss::is_all_blank(line)) {
                    continue;
                }
                else {
                    st = interpreter.eval(line);
                }
                break;

            case varlisp::Interpreter::status_ERROR:

            default:
                break;
            }
        } catch (const std::exception& e) {
            // TODO add line-number, offset
            std::cout << e.what() << std::endl;
            continue;
        }

        if (st == varlisp::Interpreter::status_OK ||
            st == varlisp::Interpreter::status_UNFINISHED) {
            if (!last_command.empty()) {
                sss::rtrim(last_command);
                last_command += " ";
            }
            // NOTE
            // 在 status_UNFINISHED
            // 状态下，清除左侧的空白，是不安全的作法——不过关系不大；
            // 我的token，可不是按行处理的。
            // 对于当前的token模式，rtrim是安全的
            sss::ltrim(line);
            last_command += line;

            if (st == varlisp::Interpreter::status_OK) {
                // Add line to history
                linenoise::AddHistory(last_command.c_str());

                // Save history
                linenoise::SaveHistory(hist_path.c_str());
                last_command.resize(0);
                indent = 0;
            }
        }
        if (st == varlisp::Interpreter::status_ERROR) {
            // NOTE 插入错误的历史，是便于之后修改……
            linenoise::AddHistory(line.c_str());
            linenoise::SaveHistory(hist_path.c_str());
            if (!last_command.empty()) {
                last_command.resize(0);
            }
            st = varlisp::Interpreter::status_OK;
        }
    }
    return EXIT_SUCCESS;
}

void help_msg()
{
    std::string app = sss::path::basename(sss::path::getbin());

    std::cout
        << app << " --help | -h\n"
        << "\t" << "print this message"
        << std::endl;

    std::cout
        << app << " [(--echo | -e) (1 | 0)]\n"
        << "\t\t" << " [--quit | -q]\n"
        << "\t\t" << " [--init | -i]\n"
        << "\t\t" << " [/path/to/script]\n\n"
        << "\t" << " 如果不提供脚步路径的话，则直接进入交互模式；"
        << "\t" << "-q "
        << std::endl;
}

bool has_match(sss::string_view tar, std::initializer_list<sss::string_view> l)
{
    for (const auto& i : l) {
        if (tar == i) {
            return true;
        }
    }
    return false;
}

int test_string_t()
{
    auto s = varlisp::String("hello world");
    std::cout << sss::raw_string(s) << std::endl;
    auto sub = s.substr(3, 4);
    std::cout << sss::raw_string(sub) << std::endl;
    std::string tmp("this is a long string from std::string");
    sub = std::move(tmp);
    std::cout << sss::raw_string(sub) << std::endl;
    std::cout << sss::raw_string(tmp) << std::endl;
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // NOTE 注册了ctrlc-handler之后，ctrl-c，就不会终止执行总的Interpret::eval()函数了。
    // 但是，这与我停止内部的asio的需求，也不符——
    // 我实际的需求是，停止当前，正在执行中的varlisp函数，然后提示符，开始下一句提示。
    // signal(SIGINT, ctrlchandler);
    // signal(SIGTERM, killhandler);

    sss::log::parse_command_line(argc, argv);
    if (!sss::colog::parse_command_line(argc, argv)) {
        sss::colog::set_log_elements(sss::colog::ls_LEVEL_SHORT |
                                     sss::colog::ls_FILE_VIM |
                                     sss::colog::ls_FUNC |
                                     sss::colog::ls_LINE);

        sss::colog::set_log_levels(sss::colog::ll_INFO |
                                   // sss::colog::ll_DEBUG |
                                   sss::colog::ll_ERROR |
                                   sss::colog::ll_WARN |
                                   sss::colog::ll_FATAL);
    }

    sss::CMLParser::RuleSingleValue cp_echo;
    sss::CMLParser::RuleSingleValue cp_quit;
    sss::CMLParser::RuleSingleValue cp_init;
    sss::CMLParser::RuleSingleValue cp_no_init;
    sss::CMLParser::RuleSingleValue cp_help;

    sss::CMLParser::Exclude cmlparser;

    cmlparser.add_rule("--echo",    sss::CMLParser::ParseBase::r_parameter, cp_echo);
    cmlparser.add_rule("-e",        sss::CMLParser::ParseBase::r_parameter, cp_echo);

    cmlparser.add_rule("--quit",    sss::CMLParser::ParseBase::r_option, cp_quit);
    cmlparser.add_rule("-q",        sss::CMLParser::ParseBase::r_option, cp_quit);

    cmlparser.add_rule("--init",    sss::CMLParser::ParseBase::r_option, cp_init);
    cmlparser.add_rule("-i",        sss::CMLParser::ParseBase::r_option, cp_init);

    cmlparser.add_rule("--no-init", sss::CMLParser::ParseBase::r_option, cp_no_init);
    cmlparser.add_rule("-n",        sss::CMLParser::ParseBase::r_option, cp_no_init);

    cmlparser.add_rule("--help",    sss::CMLParser::ParseBase::r_option, cp_help);
    cmlparser.add_rule("-h",        sss::CMLParser::ParseBase::r_option, cp_help);

    cmlparser.parse(argc, argv);

    bool echo_in_load          = false;
    bool quit_on_load_complete = false;
    bool load_init_script      = true;

    if (cp_echo.size()) {
        echo_in_load = bool(sss::string_cast<int>(cp_echo.get(0)));
    }

    if (cp_quit.size()) {
        quit_on_load_complete = true;
    }

    if (cp_init.size()) {
        load_init_script = true;
    }

    if (cp_no_init.size()) {
        load_init_script = false;
    }

    if (cp_help.size()) {
        help_msg();
        return EXIT_SUCCESS;
    }

#define CONDTION 1

#if (CONDTION==1)
    return Interpret(echo_in_load,
                     quit_on_load_complete,
                     load_init_script,
                     argc - 1, argv + 1);
#elif (CONDTION==2)
    return test_construct();
#else
    return test_other();
#endif

#undef CONDTION
}
// varLisp --sss-colog-level INFO,ERROR,WARN,FATAL --sss-colog-style LEVEL_SHORT,FUNC,LINE
// (define fib (lambda (x) (if (> x 2) (+ (fib (- x 1)) (fib (- x 2))) 1)))
// (define x (fib 20))
// (define radius 10)
// (define pi 3.14159)
// (* pi (* radius radius))
// (define circumference (* 2 pi radius))
// (quit)
// (define fibonacci (lambda (n) (define iter (lambda (i n1 n2) (if (= i 0) n2
// (iter (- i 1) n2 (+ n1 n2))))) (iter n 0 1)))
