#pragma once

#include <stdexcept>

#include <sss/util/PostionThrow.hpp>
#include <boost/variant.hpp>

#include "object.hpp"

namespace varlisp {

struct Empty;

typedef boost::variant<Empty, int, double> arithmetic_t;

inline Object arithmetic2object(const arithmetic_t& a)
{
    switch (a.which()) {
        case 0:
            return Object{};

        case 1:
            return boost::get<int>(a);

        case 2:
            return boost::get<double>(a);
    }
}

inline double arithmetic2double(const arithmetic_t& a)
{
    switch (a.which()) {
        case 0:
            SSS_POSITION_THROW(std::runtime_error,
                              "(not a valid number)");

        case 1:
            return boost::get<int>(a);

        case 2:
            return boost::get<double>(a);
    }
}

inline int arithmetic2int(const arithmetic_t& a)
{
    switch (a.which()) {
        case 0:
            SSS_POSITION_THROW(std::runtime_error,
                              "(not a valid number)");

        case 1:
            return boost::get<int>(a);

        case 2:
            {
                double ret = boost::get<double>(a);
                if (ret != int(ret)) {
                    SSS_POSITION_THROW(std::runtime_error,
                                      "(unsafe convert from", ret, "to", int(ret), ")");
                }

                return ret;
            }
    }
}

} // namespace varlisp
