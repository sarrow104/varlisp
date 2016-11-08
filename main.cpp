#include <cstdio>
#include <cstdlib>

#include <iostream>

#include <linenoise.hpp>

#include <sss/path.hpp>
#include <sss/utlstring.hpp>
#include <sss/algorithm.hpp>
#include <sss/log.hpp>
#include <sss/colorlog.hpp>

#include "varlisp/interpreter.hpp"
#include "varlisp/tokenizer.hpp"

const char * find_identifier(const char *buf)
{
    if (!buf || !buf[0]) {
        return 0;
    }
    const char * ret = std::strchr(buf, '\0');
    if (std::isspace(*(ret-1))) {
        return 0;
    }
    while (std::distance(buf, ret - 1) >= 0 && (std::isalnum(*(ret - 1)) || *(ret - 1) == '-')) {
        ret--;
    }
    return ret;
}

int get_indent(const std::string& line)
{
    int indent = 0;
    while (indent < int(line.length()) && std::isblank(line[indent])) {
        indent ++;
    }
    return indent;
}

int test_construct()
{
    // TODO
    // preload functions
    // std::string scripts = "(+ 1 2)";
    // std::string scripts = "(define a (lambda (x) (* x 2)))";
    varlisp::Interpreter interpreter;
    // interpreter.eval("(define i 0)");
    interpreter.eval("(define (sqrt-iter guess x) (if (good-enough guess x) guess (sqrt-iter (improve guess x) x)))");
    interpreter.eval("(define (improve guess x) (average guess (/ x guess)))");
    interpreter.eval("(define (average x y) (/ (+ x y) 2))");
    interpreter.eval("(define (good-enough guess x) (< (abs (- (square guess) x)) 0.001))");
    interpreter.eval("(define (sqrt x) (sqrt-iter 1.0 x))");
    interpreter.eval("(define (square x) (* x x))");
    interpreter.eval("(define (abs x) (if (< x 0) (- x) x))");
    interpreter.eval("(sqrt 9)");

    // interpreter.eval("(define fibonacci (lambda (n) (define iter (lambda (i n1 n2) (if (= i 0) n2 (iter (- i 1) n2 (+ n1 n2))))) (iter n 0 1)))");
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

int Interpret()
{
    varlisp::Interpreter interpreter;
    std::string preload_script = sss::path::dirname(sss::path::getbin());
    sss::path::append(preload_script, "init.varlisp");
    if (sss::path::filereadable(preload_script)) {
        interpreter.load(preload_script);
    }

    linenoise::SetHistoryMaxLen(100);

    // NOTE linenoise默认是单行模式；此时，大段的输入，会导致响应变慢。
    // 因此，修改为多行模式
    linenoise::SetMultiLine(true);
    linenoise::SetCompletionCallback([&interpreter](const char* editBuffer,
                                                    std::vector<std::string>& completions) {

        const char * last_identifier = find_identifier(editBuffer);
        if (!last_identifier) {
            int width = strlen(editBuffer);
            int tabsize = 8;
            // width = width - width % tabsize + tabsize;
            completions.push_back(editBuffer + std::string(tabsize - width % tabsize, ' '));
            return;
        }
        std::string prefix = std::string(editBuffer, std::distance(editBuffer, last_identifier));
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

    std::cout << hist_path << std::endl;

    // const char * hist_path = ;

    linenoise::LoadHistory(hist_path.c_str());
    std::string app = sss::path::basename(sss::path::getbin());
    varlisp::Interpreter::status_t st = varlisp::Interpreter::status_OK;

    std::string last_command;
    int indent = 0;

    while (st != varlisp::Interpreter::status_ERROR && st != varlisp::Interpreter::status_QUIT) {
        // NOTE 缩进保持功能丢失了。
        // 之前，我修改了，install 到 ~/extra/linenoise/ 下的 linenoise.hpp ，以支持 indent风格。
        // 但是，我在 pull upstream，对linenoise的更新之后，忘记我对哪里，做过修改……
        // 不过，好消息是，新的linenoise，对于tab补全遗留问题，做了修改。
        auto line = linenoise::Readline(st == varlisp::Interpreter::status_UNFINISHED ? ": " : "> ", indent);
        indent = get_indent(line);

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

        if (st == varlisp::Interpreter::status_OK || st == varlisp::Interpreter::status_UNFINISHED) {
            if (!last_command.empty()) {
                sss::rtrim(last_command);
                last_command += " ";
            }
            // NOTE
            // 在 status_UNFINISHED 状态下，清除左侧的空白，是不安全的作法——不过关系不大；
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
            if (!last_command.empty()) {
                last_command.resize(0);
            }
            st = varlisp::Interpreter::status_OK;
        }
    }
    return EXIT_SUCCESS;
}

int main (int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    sss::colog::set_log_elements(sss::colog::ls_TIME_NANO);
#if 1
    return Interpret();
#else
    return test_construct();
#endif
}

// (define fib (lambda (x) (if (> x 2) (+ (fib (- x 1)) (fib (- x 2))) 1)))
// (define x (fib 20))
// (define radius 10)
// (define pi 3.14159)
// (* pi (* radius radius))
// (define circumference (* 2 pi radius))
// (quit) 
// (define fibonacci (lambda (n) (define iter (lambda (i n1 n2) (if (= i 0) n2 (iter (- i 1) n2 (+ n1 n2))))) (iter n 0 1)))
