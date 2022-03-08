#include <cstring>
#include <vector>

#include <sss/path.hpp>
#include <sss/spliter.hpp>
#include <sss/utlstring.hpp>

#include <ss1x/asio/utility.hpp>

#include "../object.hpp"

#include "../builtin_helper.hpp"
#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/url.hpp"
#include "../json/parser.hpp"
#include "../json_print_visitor.hpp"
#include "../raw_stream_visitor.hpp"

namespace varlisp {

namespace detail {
//! http://doiob.blog.163.com/blog/static/175757412201011291023290/
//  >>> 首先,在URL中有特殊意义的字符,也就是保留字符:
//  ;    /    ?   :     @     &     =     +    $     ,        {10个}
//  这意味着,这些字符通常在URL中使用时,是有特殊含义的(如 ":"把每一个部分分隔开来),
//  如果一个URL的某一部分(如查询参数的一部分)可能包含这些字符之一,则应该在放入URL之前
//  对其进行转义处理.
//  >>>  第二组需要注意的字符集是非保留字符集.如下:
//  -  _   .   !   ~   *   '   (   )             {9个}
//  这些字符可以被用于URL的任何位置(有些地方,不允许它们出现).
//  使用它们作为URL的一部分时,你不需要进行编码/转义处理.你可以对它们进行转义操作且不影响URL
//  的语义,但不建议这么做.
// >>>  第三组  不推荐字符 也就是避用字符集合
// 使用它们是不明智的:
// {  }  |   \   ^  [   ]   `::数字1键前::       {8个}
// 不明智的原因:网关有时会修改这样的字符,或者将其作为分隔符使用.这并
// 不意味着网关总会修改这些字符,但这种情况可能发生.
// 如果真是要使用这些字符,请做转义处理.
// >>> 第四组   例外字符集
// 这组字符集是所有的ASCII控制字符组成.包含空格字符以下列字符:
// <   >   #   %   "      {5 个}
// 控制字符是不可打印的US-ASCII字符(十六进制00~1F及7F)
// 如果使用,请转义处理.有些字符#(哈希)和%(百分比)在URL上下文中有着特殊含义,你
// 可以把它们当作保留字符对待.这个集合中的其它字符无法被打印,因此对它们进行
// 转义是唯一的表示方式, <   >   "   这三个字符需要被转义，因为这些字符通常用来
// 在文本中分隔URL
//
// 编码/转义
// -------------------------
// 通常将它的ASCII十六进制值加上一个%字符．
// 如空格字符的URL编码是　%20
// %字符本身被编码为%25
//
// 这就是你所需要知道的所有URL的特殊字符，当然，从这些字符外，英文字母
// 和数字是可以直接使用而不需要进行编码的：）
//
// ！！！　必须记住
// URL应该始终保持其编码形式．只有当你要拆分URL的时候，才应该对其进行
// 解码．每个URL部分，都必须分别进行编码．
// 应该避免重复编码／解码一个URL．如果你编码一个URL一次，但解码两次，而
// 这个URL包含%字符，那么你将破坏掉你的URL
// 可以查看　RFC 2396 中的定义
// 反面的例子：
//! https://mbd.baidu.com/newspage/data/landingshare?context=%7B%22nid%22:%22news_9863627978440908763%22%7D&pageType=1&isWiseFrom=1
//! https://blog.csdn.net/u014231523/article/details/48519499
// 在url中直接使用一些特殊字符，在服务器端接收的时候经常出现数据丢失的情况。
// 那么哪些字符能够直接被服务器识别，哪些有不能呢？
// 字符“a-z”,”A-Z”,”0-9”,”.”,”-“,”_”,”*”都会直接被服务器识别，维持原值。
// 然而，字符”+”,”/”,”?”,”%”,”#”,”&”,”=”都将被转码。那么我们就需要转码来让服务器进行识别。
//
//   - “+” url中+号的表示空格 转化 %2B
//   - “/” 用来分割目录和子目录 转化 %2F
//   - “?” 用来分割请求的url和参数 转化 %3F
//   - “%” 用来指定特殊字符 转化 %25
//   - “#” 用来表示书签 转化 %23
//   - “&” 用来表示url中指定的参数间的分割符 转化%26
//   - “=” 用来表示url中指定的参数的值 转化 %3D
//   - “” url中的空格可以用+号或编码 转化%20
//
// 比如，我想让服务端识别的url为：127.0.0.1:8080/user.do?name=a12+%
// 那么，我应该在地址栏输入为：127.0.0.1:8080/user.do?name=a12%2B%25
//! https://blog.csdn.net/duyiwuer2009/article/details/47983243
// URI所允许的字符分作保留与未保留。保留字符是那些具有特殊含义的字符.
// 例如, 斜线字符用于URL (或者更一般的, URI)不同部分的分界符. 未保留字
// 符没有这些特殊含义. 百分号编码把保留字符表示为特殊字符序列。上述情
// 形随URI与URI的不同版本规格会有轻微的变化。
//
// RFC 3986 section 2.2 保留字符 (2005年1月)
//
//     ! * '( ) ; : @ & = + $ ,/ ? # [ ]
//
// RFC 3986 section 2.3 未保留字符 (2005年1月)
//
//     A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
//     a b c d e f g h i j k l m n o p q r s t u v w x y z
//     0 1 2 3 4 5 6 7 8 9 - _ . ~
//
// 即：
//
//     ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~!*'();:@&=+$,/?#[]
//
inline bool url_is_reserved_char(char ch)
{
    return std::isalnum(ch) != 0 || std::strchr("-_.~;:", ch) != nullptr;
}

// NOTE 需要处理不可打印，以及空白，还有部分符号。
struct url_encode_t
{
    sss::string_view str;
    explicit url_encode_t(const sss::string_view& s) : str(s) {}
    void print(std::ostream& o) const
    {
        for (char i : str) {
            if (detail::url_is_reserved_char(i)) {
                o << i;
            }
            else {
                o << '%' << sss::lower_hex2char(uint8_t(i) >> 4U) << sss::lower_hex2char(i);
            }
        }
    }
};

struct url_decode_t
{
    sss::string_view str;
    explicit url_decode_t(const sss::string_view& s) : str(s) {}
    void print(std::ostream& o) const
    {
        size_t i = 0;
        while (i < str.size()) {
            if (str[i] == '%') {
                if (i + 2 < str.size() && (std::isxdigit(str[i + 1]) != 0) && (std::isxdigit(str[i + 2]) != 0)) {
                    char ch = (sss::hex2int(str[i + 1]) << 4U) | sss::hex2int(str[i + 2]);
                    o << ch;
                    i += 3;
                }
                else {
                    COLOG_ERROR("unrecongnize percent-sequnce:", sss::raw_string(str.substr(i)));
                    break;
                }
            }
            else if (!detail::url_is_reserved_char(str[i])) {
                COLOG_ERROR("try to decode un-encoded:", sss::raw_char(str[i]));
                break;
            }
            else {
                o << str[i];
                ++i;
            }
        }
    }
};

std::string url_encode(const sss::string_view& str)
{
    std::ostringstream oss;
    url_encode_t(str).print(oss);
    return oss.str();
}

std::string url_decode(const sss::string_view& str)
{
    std::ostringstream oss;
    url_decode_t(str).print(oss);
    return oss.str();
}

} // namespace detail

REGIST_BUILTIN("url-split", 1, 1, eval_url_split,
               "; url-split 拆分url地址\n"
               "(url-split \"url-string\") -> '(protocal domain port path {parameters})");

// TODO NOTE url-split，并没有重用内部的串。应该考虑重用——毕竟，split后，各部
// 分只是原始串的切片。
Object eval_url_split(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-split";
    std::array<Object, 1> objs;
    const auto * p_url =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    auto parts = ss1x::util::url::split_port_auto(*p_url->gen_shared());
    auto parts_list = varlisp::List();
    parts_list.append(string_t(std::get<0>(parts)));
    parts_list.append(string_t(std::get<1>(parts)));
    parts_list.append(int64_t(std::get<2>(parts)));
    auto q_pos = std::get<3>(parts).find('?');
    std::string parameters;
    if (q_pos == std::string::npos) {
        parts_list.append(string_t(std::get<3>(parts)));
    }
    else {
        parts_list.append(string_t(std::get<3>(parts).substr(0, q_pos)));
        parameters = std::get<3>(parts).substr(q_pos + 1);
        varlisp::Environment param_env;
        if (!parameters.empty()) {
            sss::ViewSpliter<char> sp(parameters, '&');
            sss::string_view item;
            while (sp.fetch(item)) {
                auto eq_pos = item.find('=');
                if (eq_pos != sss::string_view::npos) {
                    // NOTE TODO FIXME
                    // 1. 名字部分，需要符合lisp的symbol
                    // 2. escape-sequence，应该如何处理？
                    param_env[detail::url_decode(item.substr(0, eq_pos))] = string_t(detail::url_decode(item.substr(eq_pos + 1)));
                }
                else {
                    param_env[detail::url_decode(item)] = string_t(std::string());
                }
                COLOG_DEBUG(item);
            }
        }
        parts_list.append(param_env);
    }
    return varlisp::List::makeSQuoteObj(parts_list);
}

REGIST_BUILTIN("url-join", 1, 1, eval_url_join,
               "; url-join 合并url地址\n"
               "(url-join '(protocal domain port path {parameters})) -> \"url-string\"");

Object eval_url_join(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-join";
    std::array<Object, 1> objs;
    const auto * p_list = getQuotedList(env, args.nth(0), objs[0]);
    varlisp::requireOnFaild<varlisp::QuoteList>(p_list, funcName, 1, DEBUG_INFO);
    std::string protocal;
    std::string domain;
    int port = 80;
    std::string path;
    {
        Object tmp;
        for (size_t i = 0; i < p_list->length(); ++i) {
            switch (i)
            {
                case 0: protocal = *requireTypedValue<varlisp::string_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO)->gen_shared(); break;
                case 1: domain   = *requireTypedValue<varlisp::string_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO)->gen_shared(); break;
                case 2: port     = *requireTypedValue<int64_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO); break;
                case 3: path     = *requireTypedValue<varlisp::string_t>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO)->gen_shared(); break;
                case 4:
                        {
                            const auto * p_params =
                                requireTypedValue<varlisp::Environment>(env, p_list->nth(i), tmp, funcName, i, DEBUG_INFO);
                            bool is_1st = true;
                            for (const auto & p_param : *p_params) {
                                if (is_1st) {
                                    path += '?';
                                    is_1st = false;
                                }
                                else {
                                    path += '&';
                                }
                                path += p_param.first;
                                std::ostringstream oss;
                                boost::apply_visitor(raw_stream_visitor(oss, env), p_param.second.first);
                                std::string value = oss.str();
                                if (!value.empty()) {
                                    path += '=';
                                    path += detail::url_encode(oss.str());
                                }
                            }
                        }
                        break;

                default:
                        throw std::logic_error("");
            }
        }
    }

    return string_t(ss1x::util::url::join(protocal, domain, port, path));
}

REGIST_BUILTIN("url-full", 2, 2, eval_url_full,
               "; url-full 根据base-url或者domain字符串，补全url地址\n"
               "(url-full target-string mapping-url) -> \"full-url-string\"");

Object eval_url_full(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-full";
    std::array<Object, 2> objs;
    const auto * p_target_string =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);
    const auto * p_maping_url =
        requireTypedValue<varlisp::string_t>(env, args.nth(1), objs[1], funcName, 1, DEBUG_INFO);

    auto target = p_target_string->to_string();
    bool is_modified = varlisp::detail::url::full_of(target, *p_maping_url->gen_shared());
    if (is_modified) {
        return string_t(target);
    }
    return *p_target_string;
}

REGIST_BUILTIN("url-encode", 1, 1, eval_url_encode,
               "; url-encode url地址编码。详细说明，见：\n"
               "; http://en.wikipedia.org/wiki/Percent-encoding\n"
               "; urlEncde 又名 percent-encoding；即，以百分号'%'为转移引导字符。\n"
               "; 当前，保留(不用转移)的字符有：\n"
               "; !*'();:@&=+$,/?#[]\n"
               "; 以及26个大小写字母，还有10个数字字符\n"
               "(url-encode \"string\") -> \"encoded-string\"");

Object eval_url_encode(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-encode";
    std::array<Object, 1> objs;
    const auto * p_target_string =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    return string_t(detail::url_encode(p_target_string->to_string_view()));
}

REGIST_BUILTIN("url-decode", 1, 1, eval_url_decode,
               "; url-decode 对url地址进行解码。详细说明，见：\n"
               "; http://en.wikipedia.org/wiki/Percent-encoding\n"
               "(url-decode \"encoded-string\") -> \"string\"");

Object eval_url_decode(varlisp::Environment& env, const varlisp::List& args)
{
    const char * funcName = "url-decode";
    std::array<Object, 1> objs;
    const auto * p_target_string =
        requireTypedValue<varlisp::string_t>(env, args.nth(0), objs[0], funcName, 0, DEBUG_INFO);

    return string_t(detail::url_decode(p_target_string->to_string_view()));
}

} // namespace varlisp
