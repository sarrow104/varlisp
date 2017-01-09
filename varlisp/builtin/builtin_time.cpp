#include <sstream>
#include <chrono>

#include <time.h>

#include <sss/colorlog.hpp>

#include "../object.hpp"
#include "../builtin_helper.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"

namespace varlisp {

REGIST_BUILTIN("time-elapsed", 1, 1, eval_time_elapsed,
               "; time-elapsed 执行expr 并显示操作耗时\n"
               "(time-elapsed expr) -> result-of-expr");

/**
 * @brief
 *
 * @param[in] env
 * @param[in] args
 *
 * @return
 */
Object eval_time_elapsed(varlisp::Environment &env, const varlisp::List &args)
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

REGIST_BUILTIN("time-format", 1, 2, eval_time_format,
               "; time-format\n"
               "(time-format \"fmt\" '())");

Object eval_time_format(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "time-format";
    std::array<Object, 2> objs;
    const auto * p_fmt =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    std::tm tm{0,0,0,0,0,0,0,0,0,0,0};
    if (args.length() == 2) {
        const auto * p_list = varlisp::getQuotedList(env, args.nth(1), objs[1]);
        for (size_t i = 0; i < p_list->length(); ++i) {
            Object tmp;
            switch (i) {
                case 0: tm.tm_year = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO) - 1900;
                case 1: tm.tm_mon  = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO) - 1;
                case 2: tm.tm_mday = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO);
                case 3: tm.tm_hour = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO) % 24u;
                case 4: tm.tm_min  = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO) % 60u;
                case 5: tm.tm_sec  = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO) % 60u;
            }
        }
    }
    else {
        std::tm tm = detail::get_std_tm(::std::chrono::system_clock::now());// 这个得到一个 time_point
    }
    std::string buf;
    buf.resize(p_fmt->size() + 256);
    std::string fmt = p_fmt->to_string();
    size_t cnt = std::strftime(const_cast<char*>(&buf[0]), buf.size(), fmt.c_str(), &tm);
    buf.resize(cnt);
    return string_t(std::move(buf));
}

REGIST_BUILTIN("time-strparse", 2, 2, eval_time_strparse,
               "; time-strparse 解析\n"
               "(time-strparse \"27 December 2016, at 13:17\" \"%d %h %Y, at %H:%M\") -> '(time-list)");

Object eval_time_strparse(varlisp::Environment &env, const varlisp::List &args)
{
    const char * funcName = "time-strparse";
    std::array<Object, 2> objs;
    auto source =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO)->to_string();
    auto fmt =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO)->to_string();

    std::tm tm{0,0,0,0,0,0,0,0,0,0,0};

    // "27 December 2016, at 13:17"
    // %d %h %Y, at %H:%M
    strptime(source.c_str(), fmt.c_str(), &tm);
    return varlisp::List::makeSQuoteList(int64_t(tm.tm_year + 1900), int64_t(tm.tm_mon + 1), int64_t(tm.tm_mday),
                                         int64_t(tm.tm_hour), int64_t(tm.tm_min), int64_t(tm.tm_sec));
}

} // namespace varlisp
