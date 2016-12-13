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

namespace detail {
std::tm get_std_tm(const decltype(::std::chrono::system_clock::now()) & now)
{
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    return *std::localtime(&t);
}
} // namespace detail

REGIST_BUILTIN("date", 0, 0, eval_date, "(date) -> [year month day]");
Object eval_date(varlisp::Environment &env, const varlisp::List &args)
{
    std::tm tm = detail::get_std_tm(::std::chrono::system_clock::now());// 这个得到一个 time_point
    return varlisp::List::makeSQuoteList(int64_t(tm.tm_year + 1900), int64_t(tm.tm_mon + 1), int64_t(tm.tm_mday));
}

REGIST_BUILTIN("date-time", 0, 0, eval_date_time, "(date-time) -> [year month day HH MM SS]");

Object eval_date_time(varlisp::Environment &env, const varlisp::List &args)
{
    std::tm tm = detail::get_std_tm(::std::chrono::system_clock::now());// 这个得到一个 time_point
    return varlisp::List::makeSQuoteList(int64_t(tm.tm_year + 1900), int64_t(tm.tm_mon + 1), int64_t(tm.tm_mday),
                                         int64_t(tm.tm_hour), int64_t(tm.tm_min), int64_t(tm.tm_sec));
}

REGIST_BUILTIN("date-time-nano", 0, 0, eval_date_time_nano, "(date-time-nano) -> [year month day HH MM SS nano]");

Object eval_date_time_nano(varlisp::Environment &env, const varlisp::List &args)
{
    auto now = ::std::chrono::high_resolution_clock::now();

    std::time_t t = std::chrono::system_clock::to_time_t(now);

    std::tm tm = *std::localtime(&t);// 这个得到一个 time_point

    auto duration =
        now.time_since_epoch();
    // COLOG_ERROR(duration.count());
    // auto ms =
    //     std::chrono::duration_cast<std::chrono::milliseconds>(
    //         duration % std::chrono::milliseconds(1));
    // COLOG_ERROR(ms.count());
    // auto is =
    //     std::chrono::duration_cast<std::chrono::microseconds>(
    //         duration % std::chrono::microseconds(1));
    // COLOG_ERROR(is.count());
    // auto ns =
    //     std::chrono::duration_cast<std::chrono::nanoseconds>(
    //         duration % std::chrono::nanoseconds(1));
    // COLOG_ERROR(ns.count());
    return varlisp::List::makeSQuoteList(int64_t(tm.tm_year + 1900), int64_t(tm.tm_mon + 1), int64_t(tm.tm_mday),
                                         int64_t(tm.tm_hour), int64_t(tm.tm_min), int64_t(tm.tm_sec),
                                         int64_t(duration.count() % 1000000000));
}

} // namespace varlisp
