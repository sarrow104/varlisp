#include "interpreter.hpp"
#include "parser.hpp"
#include "builtin.hpp"

#include <sss/regex/cregex.hpp>
#include <sss/log.hpp>

#include <memory>

namespace varlisp {
    Interpreter::Interpreter()
            : m_status(status_OK)
        {
            Builtin::regist_builtin_function(this->m_env);
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

    int Interpreter::retrieve_symbols(std::vector<std::string>& symbols) const
    {
        for (Environment::const_iterator it = m_env.begin(); it != m_env.end(); ++it) {
            symbols.push_back(it->first);
        }
    }

} // namespace varlisp
