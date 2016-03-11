#ifndef __INTERPRETER_HPP_1457164923__
#define __INTERPRETER_HPP_1457164923__

#include <iosfwd>

#include "environment.hpp"
#include "parser.hpp"

namespace varlisp {
    class Interpreter
    {
    public:
        enum status_t {
            status_OK,
            status_QUIT,
            status_UNFINISHED, // 指令不全
            status_ERROR,
        };

    public:
        Interpreter();
        ~Interpreter() = default;
    
    public:
        Interpreter(Interpreter&& ) = default;
        Interpreter& operator = (Interpreter&& ) = default;
    
    public:
        Interpreter(const Interpreter& ) = default;
        Interpreter& operator = (const Interpreter& ) = default;
    
    public:

        bool is_status(status_t status) const
        {
            return m_status == status;
        }

        /**
         * @brief 传入一行文本；并求值；
         *
         * @param [in] line
         *
         * @return 返回解析的状态
         */
        status_t eval(const std::string& line);
    
    private:
        status_t                m_status;
        varlisp::Environment    m_env;
        varlisp::Parser         m_parser;
    };
} // namespace varlisp


#endif /* __INTERPRETER_HPP_1457164923__ */
