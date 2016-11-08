#include "interpreter.hpp"
#include "builtin.hpp"
#include "parser.hpp"

#include <sss/log.hpp>
#include <sss/path.hpp>
#include <sss/regex/cregex.hpp>
#include <sss/utlstring.hpp>

#include <algorithm>
#include <memory>

namespace varlisp {
Interpreter::Interpreter() : m_status(status_OK)
{
    Builtin::regist_builtin_function(this->m_env);
    this->m_env.setInterpreter(*this);
}

Interpreter::status_t Interpreter::eval(const std::string& line)
{
    SSS_LOG_EXPRESSION(sss::log::log_DEBUG, line);
    status_t ret = status_OK;

    static sss::regex::CRegex reg_quit("^\\s*\\(\\s*quit\\s*\\)\\s*$");
    if (reg_quit.match(line)) {
        ret = status_QUIT;
    }
    else {
        // varlisp::Parser parser(line);
        int ec = m_parser.parse(this->m_env, line);
        if (!ec) {
            ret = status_UNFINISHED;
        }
        else if (ec < 0) {
            ret = status_ERROR;
        }
    }

    return ret;
}

void Interpreter::load(const std::string& path)
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
    this->silent(content);
    std::cout << " succeed." << std::endl;
}

void Interpreter::silent(const std::string& script)
{
    this->m_parser.parse(m_env, script, true);
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
