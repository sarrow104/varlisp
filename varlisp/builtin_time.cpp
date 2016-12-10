#include <sstream>
#include <chrono>

#include <sss/colorlog.hpp>

#include "object.hpp"
#include "builtin_helper.hpp"
#include "detail/buitin_info_t.hpp"
#include "detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("time", 1, 1, eval_time, "(time expr) -> result-of-expr");

/**
 * @brief
 *      (time expr) -> result-of-expr
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_time(varlisp::Environment &env, const varlisp::List &args)
{
    std::ostringstream exprstr;
    exprstr << detail::car(args);
    std::chrono::high_resolution_clock clock;
    std::chrono::high_resolution_clock::time_point reset_time;
    reset_time = clock.now();
    COLOG_INFO("start eval ", exprstr.str());
    Object res;
    const Object& res_ref = getAtomicValue(env, detail::car(args), res);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now() - reset_time).count();

    COLOG_INFO("end eval ", exprstr.str());
    COLOG_INFO("elapsed = ", elapsed / 1000.0);
    return res_ref;
}

REGIST_BUILTIN("date", 0, 0, eval_date, "(date) -> [year month day]");
Object eval_date(varlisp::Environment &env, const varlisp::List &args)
{
    // TODO
    return Nill{};
}

REGIST_BUILTIN("date-time", 0, 0, eval_date_time, "(date-time) -> [year month day HH MM SS]");

Object eval_date_time(varlisp::Environment &env, const varlisp::List &args)
{
    // TODO
    return Nill{};
}

REGIST_BUILTIN("date-time-nano", 0, 0, eval_date_time_nano, "(date-time-nano) -> [year month day HH MM SS nano]");

Object eval_date_time_nano(varlisp::Environment &env, const varlisp::List &args)
{
    // TODO
    return Nill{};
}

// TODO strftime(int64_t)
// int64_t parse_time(string)
// > (define l [2016 12 9])
// nil
// > l
// [2016 12 9]
// > (setq l:0 2017)
// 2017
// > l
// [2017 12 9]

} // namespace varlisp
