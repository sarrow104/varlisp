#include "interpreter.hpp"
#include "parser.hpp"
#include "builtin.hpp"

#include <sss/regex/cregex.hpp>

#include <memory>

namespace varlisp {
    Interpreter::Interpreter()
            : m_status(status_OK)
        {
            Builtin::regist_builtin_function(this->m_env);
        }

    Interpreter::status_t Interpreter::eval(const std::string& line)
    {
        status_t ret = status_OK;

        static sss::regex::CRegex reg_quit("^\\s*\\(\\s*quit\\s*\\)\\s*$");
        if (reg_quit.match(line)) {
            ret = status_QUIT;
        }
        else {
            // varlisp::Parser parser(line);
            m_parser.parse(this->m_env, line);
        }


        return ret;
    }

} // namespace varlisp
