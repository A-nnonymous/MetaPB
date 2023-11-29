#ifndef TYPE_INFER_HPP
#define TYPE_INFER_HPP
#include <string>
#include <type_traits>

namespace utils{

template<typename T>
constexpr std::string typeParse() noexcept{
  if constexpr (std::is_same_v<T, std::int32_t>){
    return "INT32";
  } else if constexpr (std::is_same_v<T, std::uint32_t>){
    return "UINT32";
  } else if constexpr (std::is_same_v<T, std::int64_t>){
    return "INT64";
  } else if constexpr (std::is_same_v<T, std::uint64_t>){
    return "UINT64";
  } else if constexpr (std::is_same_v<T, float>){
    return "FLOAT";
  } else if constexpr (std::is_same_v<T, double>){
    return "DOUBLE";
  } else {
    return "ILLEGAL";
  }
}

} // namespace utils
#endif