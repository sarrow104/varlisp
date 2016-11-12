#include "interpreter.hpp"
#include "builtin.hpp"
#include "parser.hpp"

#include <sss/log.hpp>
#include <sss/path.hpp>
#include <sss/utlstring.hpp>

#include <algorithm>
#include <memory>

namespace varlisp {
Interpreter::Interpreter() : m_status(status_OK)
{
    Builtin::regist_builtin_function(this->m_env);
    this->m_env.setInterpreter(*this);
}

Interpreter::status_t Interpreter::eval(const std::string& line, bool silent)
{
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, line);
    status_t ret = status_OK;

    // varlisp::Parser parser(line);
    int ec = m_parser.parse(this->m_env, line, silent);
    if (!ec) {
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
    for (Environment::const_iterator it = m_env.begin(); it != m_env.end();
         ++it) {
        symbols.push_back(it->first);
    }
}

int Interpreter::retrieve_symbols(std::vector<std::string>& symbols,
                                  const char* prefix) const
{
    this->m_parser.retrieve_symbols(symbols, prefix);
    for (const auto& item : this->m_env) {
        if (sss::is_begin_with(item.first, prefix)) {
            symbols.push_back(item.first);
        }
    }
    std::sort(symbols.begin(), symbols.end());
    auto last = std::unique(symbols.begin(), symbols.end());
    symbols.erase(last, symbols.end());
}

}  // namespace varlisp
