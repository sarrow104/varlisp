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
               "; [格式说明]\n"
               "; %a 星期几的简写\t %A 星期几的全称\n"
               "; %b 月分的简写  \t %B 月份的全称\n"
               "; %c 标准的日期的时间串(locale)\n"
               "; %C 年份的后两位数字 -- NOTE 不建议读取时使用\n"
               "; %d 十进制表示的每月的第几天(range 01 to 31)\n"
               "; %D 等效于 %m/%d/%y \n"
               "; %e 在两字符域中，十进制表示的每月的第几天(用空格作为fillchar)\n"
               "; %F 即(YYYY-MM-DD)，等效于 %Y-%m-%d \n"
               "; %g 年份的后两位数字，使用基于周的年(2-digit year (00-99))-- NOTE 不建议读取时使用\n"
               "; %G 年分，使用基于周的年\n"
               ";    The ISO 8601 week-based year (see NOTES) with century as a\n"
               ";    decimal number.  The 4-digit year corresponding to the ISO\n"
               ";    week number  (see %V).  This has the same format and value as\n"
               ";    %Y, except that if the ISO week number belongs to the\n"
               ";    previous or next year, that year is used instead. (TZ)\n"
               "; %h 简写的月份名; equal_to %b \n"
               "; %H 24小时制的小时;range 00 to 23 \n"
               "; %I 12小时制的小时;range 01 to 12 \n"
               "; %j 十进制表示的每年的第几天; range 001 to 366 \n"
               "; %m 十进制表示的月份(range 01 to 12)\n"
               "; %M 十时制表示的分钟数(range 00 to 59)\n"
               "; %n 新行符(A newline character)\n"
               "; %p 本地的AM或PM的等价显示(locale) Noon is treated as \"PM\" and midnight as \"AM\"\n"
               "; %P Like %p but in lowercase: \"am\" or \"pm\" or a corresponding string for the current locale.\n"
               "; %r 12小时的时间\n"
               ";    The time in a.m. or p.m. notation.\n"
               ";    In the POSIX locale this is equivalent to %I:%M:%S %p\n"
               "; %R 显示小时和分钟：hh:mm(%H:%M)\n"
               "; %S 十进制的秒数(range 00 to 60).\n"
               ";    (The range is up to 60 to allow for occasional leap seconds.)\n"
               "; %t 水平制表符(A tab character)\n"
               "; %T 显示时分秒：hh:mm:ss (%H:%M:%S)\n"
               "; %u 每周的第几天，星期一为第一天 （值从0到6，星期一为0）\n"
               "; %U 第年的第几周，把星期日做为第一天 01。（值从0到53）\n"
               "; %V 每年的第几周，使用基于周的年\n"
               ";    The ISO 8601 week number (see NOTES) of the current year as a\n"
               ";    decimal number, range 01 to 53, where week 1  is  the  first\n"
               ";    week that has at least 4 days in the new year.  See also %U\n"
               ";    and %W.\n"
               "; %w 十进制表示的星期几（值从0到6，星期天为0）\n"
               "; %W 每年的第几周，把星期一做为第一天（值从0到53）\n"
               "; %x 标准的日期串 (locale)\n"
               "; %X 标准的时间串 (locale)\n"
               "; %y 不带世纪的十进制年份（值从0到99）\n"
               "; %Y 带世纪部分的十制年份\n"
               "; %z The +hhmm or -hhmm numeric timezone (that is, the hour and minute offset from UTC)\n"
               "; %Z The timezone name or abbreviation\n"
               "; %+ The date and time in date(1) format. (TZ) (Not supported in glibc2.)\n"
               "; %% 百分号\n"
               ";\n"
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
                case 0: tm.tm_year = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO) - 1900; break;
                case 1: tm.tm_mon  = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO) - 1;    break;
                case 2: tm.tm_mday = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO);        break;
                case 3: tm.tm_hour = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO) % 24u;  break;
                case 4: tm.tm_min  = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO) % 60u;  break;
                case 5: tm.tm_sec  = *varlisp::requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO) % 60u;  break;
            }
        }
    }
    else {
        // 这个得到一个 time_point
        std::tm tm = detail::get_std_tm(::std::chrono::system_clock::now());
    }
    std::string buf;
    buf.resize(p_fmt->size() + 256);
    auto fmt = p_fmt->gen_shared();
    size_t cnt = std::strftime(const_cast<char*>(&buf[0]), buf.size(), fmt->c_str(), &tm);
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
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO)->gen_shared();
    auto fmt =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO)->gen_shared();

    std::tm tm{0,0,0,0,0,0,0,0,0,0,0};

    // "27 December 2016, at 13:17"
    // %d %h %Y, at %H:%M
    strptime(source->c_str(), fmt->c_str(), &tm);
    return varlisp::List::makeSQuoteList(int64_t(tm.tm_year + 1900), int64_t(tm.tm_mon + 1), int64_t(tm.tm_mday),
                                         int64_t(tm.tm_hour), int64_t(tm.tm_min), int64_t(tm.tm_sec));
}

} // namespace varlisp
