#pragma once

#include <string>

namespace varlisp {
namespace detail {
namespace keywords_hash {
  template<class>struct hasher;
  template<>
  struct hasher<std::string> {
    std::size_t constexpr operator()(char const *input)const {
      return *input ?
        static_cast<unsigned int>(*input) + 33 * (*this)(input + 1) :
        5381;
    }
    std::size_t operator()( const std::string& str ) const {
      return (*this)(str.c_str());
    }
  };
  template<typename T>
  std::size_t constexpr hash(T&& t) {
    return hasher< typename std::decay<T>::type >()(std::forward<T>(t));
  }
  inline namespace literals {
    std::size_t constexpr operator "" _hash(const char* s,size_t) {
      return hasher<std::string>()(s);
    }
  }
}

} // namespace detail
} // namespace varlisp
