#include "fmtArgInfo.hpp"

#include <iterator>

#include <sss/bit_operation/bit_operation.h>
#include <sss/string_view.hpp>
#include <sss/colorlog.hpp>
#include <sss/debug/value_msg.hpp>
#include <sss/util/utf8.hpp>

#include "String.hpp"

namespace varlisp {

bool fmtArgInfo::parse(Iter_t& beg, Iter_t end, size_t& last_id)
{
    rewinder_t r(beg);
    if (parser_t::parseChar(beg, end, '{')) {
        if (this->parseId(beg, end)) {
            last_id = this->index;
        }
        else {
            this->index = ++last_id;
        }
        this->parseSpec(beg, end);
        r.commit(parser_t::parseChar(beg, end, '}'));
    }
    return r.is_commited();
}

bool fmtArgInfo::parseSpec(Iter_t& beg, Iter_t end)
{
    rewinder_t r(beg);
    if (parser_t::parseChar(beg, end, ':')) {
        char tmp_fill = ' ';
        rewinder_t r2(beg);
        r2.commit((parser_t::parseIfChar(
                       beg, end, static_cast<int (*)(int)>(&std::isalnum),
                       tmp_fill) ||
                   parser_t::parseSetChar(beg, end, " ", tmp_fill)) &&
                  this->parseAlign(beg, end)) ||
            r2.commit(this->parseAlign(beg, end));
        if (r2.is_commited()) {
            this->fill = tmp_fill;
        }
        r2.end();
        this->parseSign(beg, end);
        this->parseWidgh(beg, end);
        this->parsePrecision(beg, end);
        this->parseType(beg, end);
        r.commit(true);
    }
    return r.is_commited();
}
bool fmtArgInfo::parseId(Iter_t& beg, Iter_t end)
{
    uint32_t id;
    bool is_ok = parser_t::parseUint32_t(beg, end, id);
    if (is_ok) {
        this->index = id;
    }
    return is_ok;
}
bool fmtArgInfo::parseAlign(Iter_t& beg, Iter_t end)
{
    return parser_t::parseSetChar(beg, end, "<=>^", this->align);
};
bool fmtArgInfo::parseSign(Iter_t& beg, Iter_t end)
{
    return parser_t::parseSetChar(beg, end, "+- ", this->sign);
};
bool fmtArgInfo::parseWidgh(Iter_t& beg, Iter_t end)
{
    uint32_t w;
    bool is_ok = parser_t::parseUint32_t(beg, end, w);
    if (is_ok) {
        this->width = w;
    }
    return is_ok;
};
bool fmtArgInfo::parsePrecision(Iter_t& beg, Iter_t end)
{
    rewinder_t r(beg);
    uint32_t p = 0;
    r.commit(parser_t::parseChar(beg, end, '.') &&
             parser_t::parseUint32_t(beg, end, p));
    if (r.is_commited()) {
        this->precision = p;
    }
    return r.is_commited();
};
bool fmtArgInfo::parseType(Iter_t& beg, Iter_t end)
{
    return parser_t::parseSetChar(beg, end, "bcdeEfFgGnosxX%", this->type);
}

void parseFmt(const string_t* p_fmt, std::vector<fmtArgInfo>& fmts,
              std::vector<sss::string_view>& padding)
{
    sss::string_view sv_fmt = p_fmt->to_string_view();
    const char* fmt_ini = sv_fmt.begin();
    const char* fmt_fin = sv_fmt.end();
    size_t ref_id = sss::string_view::npos;
    const char* current = fmt_ini;
    int loop_id = 0;
    while (!sv_fmt.empty()) {
        while (current != fmt_fin && *current != '{' && *current != '}') {
            current++;
        }
        size_t offset = current - sv_fmt.data();
        COLOG_DEBUG(loop_id, SSS_VALUE_MSG(offset));
        COLOG_DEBUG(loop_id, SSS_VALUE_MSG(sv_fmt.substr(offset)));
        COLOG_DEBUG(loop_id,
                    SSS_VALUE_MSG(sv_fmt.substr(offset).is_begin_with("{{")));
        if (sv_fmt.substr(offset).is_begin_with("{{")) {
            padding.push_back(sv_fmt.substr(0, offset + 1));
            current += 2;
            sv_fmt.remove_prefix(offset + 2);
            fmts.push_back(fmtArgInfo{});
            COLOG_DEBUG(loop_id, SSS_VALUE_MSG(padding.back()));
            COLOG_DEBUG(loop_id, SSS_VALUE_MSG(fmts.back()));
        }
        else if (sv_fmt.substr(offset).is_begin_with("{")) {
            fmtArgInfo info;
            if (!parseFmtInfo(current, fmt_fin, info, ref_id)) {
                SSS_POSITION_THROW(std::runtime_error,
                                  sss::raw_string(sss::string_view{
                                      current, size_t(fmt_fin - current)}),
                                  " error!");
            }
            padding.push_back(sv_fmt.substr(0, offset));
            fmts.push_back(info);
            COLOG_DEBUG(loop_id, SSS_VALUE_MSG(padding.back()));
            COLOG_DEBUG(loop_id, SSS_VALUE_MSG(fmts.back()));
            sv_fmt.remove_prefix(current - sv_fmt.data());
        }
        else if (sv_fmt.substr(offset).is_begin_with("}}")) {
            padding.push_back(sv_fmt.substr(0, offset + 1));
            current += 2;
            sv_fmt.remove_prefix(offset + 2);
            fmts.push_back(fmtArgInfo{});
            COLOG_DEBUG(loop_id, SSS_VALUE_MSG(padding.back()));
            COLOG_DEBUG(loop_id, SSS_VALUE_MSG(fmts.back()));
        }
        else if (sv_fmt.substr(offset).is_begin_with("}")) {
            SSS_POSITION_THROW(std::runtime_error, "encounter single '}'");
        }
        else {
            padding.push_back(sv_fmt.substr(0, offset));
            COLOG_DEBUG(loop_id, SSS_VALUE_MSG(padding.back()));
            break;
        }
        loop_id++;
    }
}

void fmtArgInfo::fillN(std::ostream& o, char fill, size_t n) const
{
    COLOG_DEBUG(SSS_VALUE_MSG(n));
    for (size_t i = 0; i < n; ++i) {
        o.put(fill);
    }
}

void fmtArgInfo::print(std::ostream& o, const sss::string_view& s) const
{
    if (this->width > s.size()) {
        char align = this->align ? this->align : '<';
        switch (align) {
            case '<':
                o.write(s.data(), s.size());
                this->fillN(o, this->fill, this->width - s.size());
                break;
            case '=':
                this->fillN(o, this->fill, this->width - s.size());
                o.write(s.data(), s.size());
                break;

            case '>':
            case '^':
                this->fillN(o, this->fill, (this->width - s.size()) / 2);
                o.write(s.data(), s.size());
                this->fillN(o, this->fill, this->width - (this->width - s.size()) / 2);
                break;
        }
    }
    else {
        o.write(s.data(), s.size());
    }
}

void fmtArgInfo::adjust(std::ostream& o, sss::string_view s, char sign,
                        char fill, char align, size_t width) const
{
    COLOG_DEBUG(s, sign, fill, align, width);
    int byte_cnt = 0;
    bool add_plus = sign == '+' && s.front() != '-' && s.front() != '+';
    int prev_padd = 0;
    switch (align) {
        case '<':
            if (add_plus) {
                o.put('+');
                byte_cnt++;
            }
            o.write(s.data(), s.size());
            byte_cnt += s.size();
            if (byte_cnt < int(width)) {
                this->fillN(o, fill, width - byte_cnt);
            }
            break;

        case '=':
            if (width > add_plus ? 1 : 0 + s.size()) {
                prev_padd = int(width - (add_plus ? 1 : 0) - s.size()) / 2;
                this->fillN(o, fill, prev_padd);
            }
            if (add_plus) {
                o.put('+');
            }
            o.write(s.data(), s.size());
            if (width > add_plus ? 1 : 0 + s.size()) {
                this->fillN(o, fill, int(width - (add_plus ? 1 : 0) - s.size() -
                                         prev_padd));
            }
            break;

        case '^':
            if (add_plus) {
                o.put('+');
                width--;
            }
            if (s.front() == '-') {
                o.put('-');
                s.remove_prefix(1);
                width--;
            }
            if (width > s.size()) {
                this->fillN(o, fill, width - s.size());
            }
            o.write(s.data(), s.size());
            break;

        case '>':
            COLOG_DEBUG(SSS_VALUE_MSG(width));
            if (width > (add_plus ? 1 : 0) + s.size()) {
                this->fillN(o, fill, width - (add_plus ? 1 : 0) - s.size());
            }
            if (add_plus) {
                o.write(&sign, 1);
            }
            o.write(s.data(), s.size());
            break;
    }
}

void fmtArgInfo::print(std::ostream& o, double f) const
{
    char buf[32];
    char c_fmt[32];
    char type = 'f';
    if (this->type) {
        type = this->type;
    }
    char align = '>';
    if (this->align) {
        align = this->align;
    }
    char sign = '-';
    if (this->sign) {
        sign = this->sign;
    }
    switch (type) {
        case 'c': {
            int32_t i = f;
            sss::util::utf8::dumpout2utf8(&i, &i + 1,
                                          std::ostream_iterator<char>(o));
            break;
        }

        case 'b': {
            std::ostringstream oss;
            ext::binary(oss) << f;
            this->print(o, oss.str());
            break;
        }

        case 'e':
        case 'E':
        case 'f':
        case 'F':
        case 'g':
        case 'G':
            std::sprintf(c_fmt, "%%.%d%c", int(this->precision), type);
            std::sprintf(buf, c_fmt, f);
            adjust(o, buf, sign, this->fill, align, this->width);
            break;

        case '%':
            std::sprintf(c_fmt, "%%.%d%c%%%%", 2, 'f');
            adjust(o, buf, sign, this->fill, align, this->width);
            break;

        default:
            std::sprintf(c_fmt, "%%.%df", int(this->precision));
            std::sprintf(buf, c_fmt, f);
            adjust(o, buf, sign, this->fill, align, this->width);
            break;
    }
}

// void fmtArgInfo::print(std::ostream& o, float f) const
// {
// }

void fmtArgInfo::print(std::ostream& o, bool b) const
{
    this->print(o, b ? "ture" : "false");
}

void fmtArgInfo::print(std::ostream& o, int32_t i) const
{
    char buf[32];
    char c_fmt[32];
    char type = 'd';
    if (this->type) {
        type = this->type;
    }
    char align = '>';
    if (this->align) {
        align = this->align;
    }
    char sign = '-';
    if (this->sign) {
        sign = this->sign;
    }

    switch (this->type) {
        case 'c': {
            sss::util::utf8::dumpout2utf8(&i, &i + 1,
                                          std::ostream_iterator<char>(o));
        } break;

        case 'b': {
            std::ostringstream oss;
            ext::binary(oss) << i;
            this->print(o, oss.str());
            break;
        }

        case 'd':
        case 'o':
        case 'x':
        case 'X':
            std::sprintf(c_fmt, "%%%c", this->type);
            std::sprintf(buf, c_fmt, i);
            adjust(o, buf, sign, this->fill, align, this->width);
            break;

        default:
            std::sprintf(buf, "%d", i);
            adjust(o, buf, sign, this->fill, align, this->width);
            break;
    }
}

}  // namespace varlisp
