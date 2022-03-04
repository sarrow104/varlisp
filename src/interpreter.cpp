#include "interpreter.hpp"

#include <algorithm>
#include <memory>

#include <sss/log.hpp>
#include <sss/path.hpp>
#include <sss/utlstring.hpp>

#include "builtin.hpp"
#include "parser.hpp"

namespace varlisp {
Interpreter::Interpreter() : m_status(status_OK)
{
    Builtin::regist_builtin_function(this->m_env);
}

Interpreter::status_t Interpreter::eval(const std::string& line, bool silent)
{
    try {
        SSS_LOG_EXPRESSION(sss::log::log_DEBUG, line);
        status_t ret = status_OK;

        // varlisp::Parser parser(line);
        int ec = m_parser.parse(this->m_env, line, silent);
        if (ec == 0) {
            ret = status_UNFINISHED;
        }
        else if (ec < 0) {
            ret = status_ERROR;
        }

        if (m_status != status_QUIT) {
            m_status = ret;
        }
        return m_status;
    }
    catch (Object& exception) {
        COLOG_ERROR("unhandled exception: ", exception);
        m_status = status_ERROR;
        return m_status;
    }
}

Interpreter& Interpreter::get_instance()
{
    static Interpreter g_interpreter;
    return g_interpreter;
}

void Interpreter::load(const std::string& path, bool echo)
{
    std::string full_path = sss::path::full_of_copy(path);
    if (!sss::path::filereadable(full_path)) {
        std::cerr << "load " << path << " failed." << std::endl;
    }
    std::string content;
    sss::path::file2string(full_path, content);
    if (content.empty()) {
        return;
    }
    std::cout << "loading " << full_path << " ...";
    this->eval(content, !echo);
    std::cout << " succeed." << std::endl;
}

int Interpreter::retrieve_symbols(std::vector<std::string>& symbols) const
{
    int cnt = 0;
    for (auto it = m_env.begin(); it != m_env.end(); ++cnt, ++it) {
        symbols.push_back(it->first);
    }
    return cnt;
}

int Interpreter::retrieve_symbols(std::vector<std::string>& symbols,
                                  const char* prefix) const
{
    int cnt = 0;
    this->m_parser.retrieve_symbols(symbols, prefix);
    for (const auto& item : this->m_env) {
        if (sss::is_begin_with(item.first, prefix)) {
            symbols.push_back(item.first);
            ++cnt;
        }
    }
    std::sort(symbols.begin(), symbols.end());
    auto last = std::unique(symbols.begin(), symbols.end());
    symbols.erase(last, symbols.end());
    return cnt;
}

}  // namespace varlisp
