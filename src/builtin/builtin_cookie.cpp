// cookie管理
// 模拟setcookie,getcookie这两个函数。
//! http://blog.csdn.net/talking12391239/article/details/9665185
//
// Cookie由变量名和值组成，类似Javascript变量。其属性里既有标准的Cookie变量，也
// 有用户自己创建的变量，属性中变量是用“变量=值”形式来保存。
//
//　　根据Netscape公司的规定，Cookie格式如下：
//
//　　Set－Cookie: NAME=VALUE；Expires=DATE；Path=PATH；Domain=DOMAIN_NAME；
// SECURE
//
//　　NAME=VALUE：
//
//　　这是每一个Cookie均必须有的部分。NAME是该Cookie的名称，VALUE是该Cookie的值
//。在字符串“NAME=VALUE”中，不含分号、逗号和空格等字符。
//
//　　Expires=DATE：Expires变量是一个只写变量，它确定了Cookie有效终止日期。该属
//性值DATE必须以特定的格式来书写：星期几，DD－MM－YY HH:MM:SS GMT，GMT表示这是
//格林尼治时间。反之，不以这样的格式来书写，系统将无法识别。该变量可省，如果缺
//省时，则Cookie的属性值不会保存在用户的硬盘中，而仅仅保存在内存当中，Cookie文
//件将随着浏览器的关闭而自动消失。
//
//　　Domain=DOMAIN－NAME:Domain该变量是一个只写变量，它确定了哪些Internet域中
//的Web服务器可读取浏览器所存取的Cookie，即只有来自这个域的页面才可以使用Cookie
//中的信息。这项设置是可选的，如果缺省时，设置Cookie的属性值为该Web服务器的域名
//。
//
//　　Path=PATH：Path属性定义了Web服务器上哪些路径下的页面可获取服务器设置的
// Cookie。一般如果用户输入的URL中的路径部分从第一个字符开始包含Path属性所定义的
//字符串，浏览器就认为通过检查。如果Path属性的值为“/”，则Web服务器上所有的WWW
//资源均可读取该Cookie。同样该项设置是可选的，如果缺省时，则Path的属性值为Web服
//务器传给浏览器的资源的路径名。
//
//　　可以看出我们借助对Domain和Path两个变量的设置，即可有效地控制Cookie文件被
//访问的范围。
//
//　　Secure：在Cookie中标记该变量，表明只有当浏览器和Web Server之间的通信协议
//为加密认证协议时，浏览器才向服务器提交相应的Cookie。当前这种协议只有一种，即
//为HTTPS。
//
//    Cookies以键值的方式记录会话跟踪的内容.服务器利用响应报头Set-Cookie
// 来发送COOKIE信息.在RFC2109中定义的SET-COOKIE响应报头的格式为:
//
// Set-Cookie: Name = Value; Comment = value; Domain = value; Max-Age = value;
// Path = Value; Secure; Version = 1 * DIGIT;
//
// Name是Cookie的名字
// Value是它的值
// Name=Value属性值对必须首先出现,在此之后的属性-值对可以以任何顺序出现.
// 在Servlet规范中,用于会话跟踪的cookie的名字必须是JSESSIONID
// Comment属性是可选的,因为Cookie可能包含其它有关用户私有的信息.这个属性允许服务器说明这个Cookie的使用,用户可以检查这个消息,然话决定是否加入或继续会话.
// Domain属性也是可选的.它用来指定Cookie在哪一个域中有效.所指定的域必须以点号(.)来开始.
// Max-Age属性是可选的,用于定义Cookie的生存时间,以秒为单位.
// 如果超过了这个时间,客户端就应该丢弃这个cookie.
// 如果指定的秒数为0,表示这个cookie应立即被丢弃.
// Path属性是可选的,用于指定这个cookie在哪一个URL子集下有效.
// Secure属性是必需的,它的值是一个十进制数,标识cookie依照的状态管理规范的版本.
//
// 例如:
// set-cookie: uid = zhangsan; Max-Age=3600; Domain=.sun.org; Path=/bbs;
// Version=1
//
// 它表示一个名为uid,值为zhangsan的cookie.生存时间为3600秒,在sunxin.org域的
// bbs路径下有效.
//
// 在3600秒后,客户端将抛弃这个cookie.
//
// 当IE收到上面这个响应报头后,可以选择接受或拒绝这个cookie.如果ID接受了这个
// cookie,当浏览器下一次发送请求到http://www.sunxin.org/bbs路径下的资源时,同时
// 也会发送以下的请求报头:
//
// cookie:uid=zhangsan.
//
//  1. domain表示的是cookie所在的域，默认为请求的地址，如网址为
//  www.test.com/test/test.aspx，那么domain默认为www.test.com。而跨域访问，如域
//  A为t1.test.com，域B为t2.test.com，那么在域A生产一个令域A和域B都能访问的
//  cookie就要将该cookie的domain设置为.test.com；如果要在域A生产一个令域A不能访
//  问而域B能访问的cookie就要将该cookie的domain设置为t2.test.com。
//
//  2. path表示cookie所在的目录，asp.net默认为/，就是根目录。在同一个服务器上有
//  目录如下：/test/,/test/cd/,/test/dd/，现设一个cookie1的path为/test/，
//  cookie2的path为/test/cd/，那么test下的所有页面都可以访问到cookie1，而/test/
//  和/test/dd/的子页面不能访问cookie2。这是因为cookie能让其path路径下的页面访
//  问。
//
//  3. 浏览器会将domain和path都相同的cookie保存在一个文件里，cookie间用*隔开。
//
//  4. 含值键值对的cookie：以前一直用的是nam=value单键值对的cookie，一说到含多
//  个子键值对的就蒙了。现在总算弄清楚了。含多个子键值对的cookie格式是
//  name=key1=value1&key2=value2。可以理解为单键值对的值保存一个自定义的多键值
//  字符串，其中的键值对分割符为&，当然可以自定义一个分隔符，但用asp.net获取时
//  是以&为分割符。

#include "../object.hpp"

#include "../builtin_helper.hpp"

#include "../detail/buitin_info_t.hpp"
#include "../detail/car.hpp"
#include "../detail/cookie.hpp"

namespace varlisp {

REGIST_BUILTIN("cookie-enable", 1, 1, eval_cookie_enable,
               "; cookie-enable 开启关闭cookie\n"
               "(cookie-enable boolean) -> boolean");

Object eval_cookie_enable(varlisp::Environment& env, const varlisp::List& args)
{
    const char* funcName = "cookie-enable";
    Object tmp;
    const auto* p_bool = varlisp::requireTypedValue<bool>(
        env, detail::car(args), tmp, funcName, 0, DEBUG_INFO);
    detail::CookieMgr_t::set_cookie_enable_status(*p_bool);
    return *p_bool;
}

REGIST_BUILTIN("cookie-enable?", 0, 0, eval_cookie_enable_q,
               "; cookie-enable? 显示当前cookie开启关闭状态\n"
               "(cookie-enable?) -> boolean");

Object eval_cookie_enable_q(varlisp::Environment&  /*env*/,
                            const varlisp::List&  /*args*/)
{
    return detail::CookieMgr_t::get_cookie_enable_status();
}

REGIST_BUILTIN(
    "cookie-get-value", 2, 2, eval_cookie_get_value,
    "; cookie-get-value 获取cookie\n"
    "(cookie-get-value \"domain\" \"path\") -> \"cookie-value\" | nil");

Object eval_cookie_get_value(varlisp::Environment& env,
                             const varlisp::List& args)
{
    const char* funcName = "cookie-get-value";
    std::array<Object, 2> tmp;
    const auto* p_domain = varlisp::requireTypedValue<varlisp::string_t>(
        env, args.nth(0), tmp[0], funcName, 0, DEBUG_INFO);
    const auto* p_path = varlisp::requireTypedValue<varlisp::string_t>(
        env, args.nth(1), tmp[1], funcName, 1, DEBUG_INFO);

    std::string cookie = detail::CookieMgr_t::getCookie(*p_domain->gen_shared(),
                                                        *p_path->gen_shared());
    if (cookie.empty()) {
        return Nill{};
    }
    return string_t(cookie);
}

REGIST_BUILTIN("cookie-set-value", 3, 3, eval_cookie_set_value,
               "; cookie-set-value 设置cookie\n"
               "(cookie-set-value \"domain\" \"path\" \"cookie\") -> boolean");

Object eval_cookie_set_value(varlisp::Environment& env,
                             const varlisp::List& args)
{
    const char* funcName = "cookie-get-value";
    std::array<Object, 3> tmp;
    const auto* p_domain = varlisp::requireTypedValue<varlisp::string_t>(
        env, args.nth(0), tmp[0], funcName, 0, DEBUG_INFO);
    const auto* p_path = varlisp::requireTypedValue<varlisp::string_t>(
        env, args.nth(1), tmp[1], funcName, 1, DEBUG_INFO);
    const auto* p_cookie = varlisp::requireTypedValue<varlisp::string_t>(
        env, args.nth(2), tmp[2], funcName, 2, DEBUG_INFO);

    return detail::CookieMgr_t::setCookie(
        *p_domain->gen_shared(), *p_path->gen_shared(), *p_cookie->gen_shared());
}

}  // namespace varlisp
