#include "environment.hpp"

#include <sss/util/Memory.hpp>

namespace varlisp {

    // Environment::~Environment()
    // {
    //     for (BaseT::const_iterator it = this->begin(); it != this->end(); ++it) {
    //         delete it->second;
    //     }
    // }

    // varlisp::Node * Environment::getValue(const std::string& name) const
    // {
    //     BaseT::const_iterator it = this->BaseT::find(name);
    //     if (it == this->BaseT::cend()) {
    //         return 0;
    //     }
    //     return it->second;
    // }

    // void Environment::putValue(const std::string& name, varlisp::Node * value)
    // {
    //     if (value) {
    //         std::swap(this->BaseT::operator[](name), value);
    //         sss::scoped_ptr<varlisp::Node> scop(value);
    //     }
    // }
} // namespace varlisp
