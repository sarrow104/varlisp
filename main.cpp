#include <cstdio>
#include <cstdlib>

#include <iostream>

#include <linenoise.hpp>

#include <sss/path.hpp>
#include <sss/utlstring.hpp>
#include <sss/algorithm.hpp>
#include <sss/log.hpp>

#include "varlisp/interpreter.hpp"
#include "varlisp/tokenizer.hpp"

int main (int argc, char *argv[])
{
    (void) argc;
    (void) argv;

#if 1
    varlisp::Interpreter interpreter;

    linenoise::SetHistoryMaxLen(100);
    linenoise::SetCompletionCallback([](const char* editBuffer,
                                            std::vector<std::string>& completions) {
        completions.push_back("(lambda ");
        completions.push_back("(if ");
        completions.push_back("(list ");
        completions.push_back("(define ");
        completions.push_back("(quit) ");
    });

    std::string hist_path = sss::path::dirname(sss::path::getbin());
    sss::path::append(hist_path, "history.txt");

    std::cout << hist_path << std::endl;

    sss::log::level(sss::log::log_ERROR);
    // const char * hist_path = ;

    linenoise::LoadHistory(hist_path.c_str());
    std::string app = sss::path::basename(sss::path::getbin());
    varlisp::Interpreter::status_t st = varlisp::Interpreter::status_OK;
    while (st != varlisp::Interpreter::status_ERROR && st != varlisp::Interpreter::status_QUIT) {
        auto line = linenoise::Readline("> ");

        // 如何处理空行？
        // 当然Interpreter的状态是status_UNFINISHED的时候，不对line进行处理，直
        // 接eval即可；
        //
        // status_OK状态，则直接continue；
        //
        // status_UNFINISHED  任何行，都eval()
        //
        // status_OK          避开空行；

        switch (st) {
        case varlisp::Interpreter::status_QUIT:
            std::cout << "quit! bye" << std::endl;
            exit(EXIT_SUCCESS);

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
            break;
        }

        // Add line to history
        linenoise::AddHistory(line.c_str());

        // Save history
        linenoise::SaveHistory(hist_path.c_str());
    }

#else
    // std::string scripts = "(+ 1 2)";
    // std::string scripts = "(define a (lambda (x) (* x 2)))";
    varlisp::Interpreter interpreter;
    interpreter.eval("(define fib (lambda (x) (if (> x 2) (+ (fib (- x 1)) (fib (- x 2))) 1)))");
    interpreter.eval("(define x 3)");
    interpreter.eval("(- x 1)");
    interpreter.eval("(> x 2)");
    interpreter.eval("(fib (- x 1))");
    interpreter.eval("(fib 5)");
    // interpreter.eval("(fib 1)");
    // interpreter.eval("(fib 2)");
    // interpreter.eval("(fib 3)");
    return EXIT_SUCCESS;
#endif
    return EXIT_SUCCESS;
}

