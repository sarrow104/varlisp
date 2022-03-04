#ifndef __INTERPRETER_HPP_1457164923__
#define __INTERPRETER_HPP_1457164923__

#include <iosfwd>

#include "environment.hpp"
#include "parser.hpp"

namespace varlisp {
class Interpreter {
public:
    enum status_t {
        status_OK,
        status_QUIT,
        status_UNFINISHED,  // 指令不全
        status_ERROR,
    };

protected:
    Interpreter();
    ~Interpreter() = default;

protected:
    Interpreter(Interpreter&&) = default;
    Interpreter& operator=(Interpreter&&) = default;

protected:
    Interpreter(const Interpreter&) = default;
    Interpreter& operator=(const Interpreter&) = default;

public:
    bool is_status(status_t status) const { return m_status == status; }
    varlisp::Parser& get_parser() { return this->m_parser; }
    void load(const std::string& path, bool echo = false);

    int retrieve_symbols(std::vector<std::string>& symbols) const;
    int retrieve_symbols(std::vector<std::string>& symbols,
                         const char* prefix) const;

    void set_status(status_t s) {
        m_status = s;
    }

    varlisp::Environment& get_env() {
        return m_env;
    }
    /**
     * @brief 传入一行文本；并求值；
     *
     * @param [in] line
     *
     * @return 返回解析的状态
     */
    status_t eval(const std::string& line, bool silent = false);

    static Interpreter& get_instance();
private:
    status_t m_status;
    varlisp::Environment m_env;
    varlisp::Parser m_parser;
};
}  // namespace varlisp

#endif /* __INTERPRETER_HPP_1457164923__ */
