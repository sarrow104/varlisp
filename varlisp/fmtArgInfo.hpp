#pragma once

#include <cstddef>

#include <sss/util/Parser.hpp>

namespace varlisp {
// place holder syntax "{1}{2}{3}
// 好像没法直接使用 fmt 库。
// 1. fmt库不是输出到std::cout的。而是到std::string
// 2. fmt库是c printf调用样式。而我的eval_xxxx函数，是一个一个抽取的Object类型。
// 不方便直接讲所有需要序列化的参数，写在一起。
// 触发使用汇编。
//
// 综上，我应该自己实现，fmt-str的拆分，以及参数变量的获取。另外，效率考虑，应该使用延迟计算的方式；
// 先创建一个std::vector<const Object*>的数组。
// 然后，根据需要，eval对应位置的参数，然后序列化到流里面。——以便同时使用
//
//! https://docs.python.org/2/library/string.html
//! http://www.2cto.com/kf/201312/262068.html
// Python中格式化输出字符串使用format()函数, 字符串即类, 可以使用方法;
//
// Python是完全面向对象的语言, 任何东西都是对象;
//
// 字符串的参数使用{NUM}进行表示,0, 表示第一个参数,1, 表示第二个参数,
// 以后顺次递加;
//
// 使用":", 指定代表元素需要的操作, 如":.3"小数点三位, ":8"占8个字符空间等;
//
// 还可以添加特定的字母, 如:
//
// 'b' - 二进制. 将数字以2为基数进行输出.
//
// 'c' - 字符. 在打印之前将整数转换成对应的Unicode字符串.
//
// 'd' - 十进制整数. 将数字以10为基数进行输出.
//
// 'o' - 八进制. 将数字以8为基数进行输出.
//
// 'x' - 十六进制. 将数字以16为基数进行输出, 9以上的位数用小写字母.
//
// 'e' - 幂符号. 用科学计数法打印数字, 用'e'表示幂.
//
// 'g' - 一般格式. 将数值以fixed-point格式输出. 当数值特别大的时候,
// 用幂形式打印.
//
// 'n' - 数字. 当值为整数时和'd'相同, 值为浮点数时和'g'相同.
// 不同的是它会根据区域设置插入数字分隔符.
//
// '%' - 百分数. 将数值乘以100然后以fixed-point('f')格式打印,
// 值后面会有一个百分号.
//
// 数字(0, 1, ...)即代表format()里面的元素, 所以可以使用"."调用元素的方法;
// ----------------------------------------------------------------------
// 另外，从运算效率来看，应该先判断格式正确性，然后再对参数求值。

// format_spec ::=  [[fill]align][sign][#][0][width][,][.precision][type]
// fill        ::=  <any character>
// align       ::=  "<" | ">" | "=" | "^"
//   '=' ->  ‘+000000120’
// sign        ::=  "+" | "-" | " "
//   '+' 始终添加正负号；
//   '-' 仅负数添加负号；
//   ' ' 正数的正号，用空格代替——以保证正负数，尽量对齐……
// width       ::=  integer
// precision   ::=  integer
//   仅对浮点数有用；
// type        ::=  "b" | "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "n" |
// "o" | "s" | "x" | "X" | "%"
//   'b' 二进制；
//   'c' 将数字，在打印前，转换成对应的unicode字符；
//   'd' 十进制数字；
//   'o' 八进制数字；
//   'x' 十六进制；小写；
//   'X' 十六进制，大写
//   'n' 同'd'；但是用local来打印，比如按1000进位，用逗号分割；
//   'e' 浮点数指数，小写；
//   'E' 浮点数指数，大写；
//   'f' 固定小数点；默认精度6；
//   'F' 同'f'
//   'g' 科学计数法
//   'G' 科学计数法；大写'E'；
//   '%' 百分号形式，打印浮点数；附带'%'

struct fmtArgInfo {
    fmtArgInfo() : index(-1) {}
    ~fmtArgInfo() = default;
    size_t index = 0u; // 变量引用下标；从0开始；没有显示提供下标，则当前下标为前一个"捕获"的下标+1；
    char fill = ' ';  // 填充符号；默认为空格；
    char align = '\0'; // 水平对齐方式；默认是字符串居左，数字居右。 '<','>','=','^'
    char sign = '\0';  // 符号位；数字默认'-'
    char type = '\0';  // 显示风格
    size_t width = 0u;
    size_t precision = 6u;  // 精度；用于数字打印；小数的宽度。
    // size_t min_length; //
    // 最小长度；不足的部分，会用fill填充；如果为0，则无填充；
    // size_t max_length; // 最大长度；超出的部分会阶段；如果为0，则无截断限制。
    typedef sss::string_view::const_iterator Iter_t;
    typedef sss::util::Parser<Iter_t> parser_t;
    typedef parser_t::Rewinder rewinder_t;
    bool parse(Iter_t &beg, Iter_t end, size_t& last_id);
    bool parseSpec(Iter_t &beg, Iter_t end);
    bool parseId(Iter_t &beg, Iter_t end);
    bool parseAlign(Iter_t &beg, Iter_t end);
    bool parseSign(Iter_t &beg, Iter_t end);
    bool parseWidgh(Iter_t &beg, Iter_t end);
    bool parsePrecision(Iter_t &beg, Iter_t end);
    bool parseType(Iter_t& beg, Iter_t end);

    void print(std::ostream& o, const sss::string_view& s) const;
    void print(std::ostream& o, double lf) const;
    // void print(std::ostream& o, float f) const;
    void print(std::ostream& o, bool f) const;
    void print(std::ostream& o, int32_t i) const;
    // void print(std::ostream& o, uint32_t i) const;

    void fillN(std::ostream& o, char fill, size_t n) const;
    void adjust(std::ostream& o, sss::string_view s, char sign, char fill, char align, size_t width) const;
};

inline std::ostream& operator << (std::ostream& o, const fmtArgInfo& f)
{
    o << '(' << f.index << ", " << f.fill << ", " << f.sign << ", " << f.type << ", " << f.width << ", " << f.precision << ')';
    return o;
}

inline bool parseFmtInfo(const char*& current, const char* end, fmtArgInfo& info,
                  size_t& last_id)
{
    return info.parse(current, end, last_id);
}

struct String;
typedef String string_t;

// 将格式串
// fmt，解析为padding与fmts间隔的两个数组。假设以padding开始，并以padding结尾。
// 如果发生了解析异常，则抛出错误。
void parseFmt(const string_t* p_fmt, std::vector<fmtArgInfo>& fmts,
              std::vector<sss::string_view>& padding);

} // namespace varlisp


