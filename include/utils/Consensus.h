#ifndef CONSENSUS_HPP
#define CONSENSUS_HPP

// This enum shares the consensus of datatype between host & dpu
#define UCHAR8_32ALN 0   // unsigned char (uint8_t)
#define CHAR8_32ALN 1    // char (int8_t)
#define USHORT16_32ALN 2 // unsigned short (uint16_t)
#define SHORT16_32ALN 3  // short (int16_t)
#define UINT32_32ALN 4   // unsigned int (uint32_t)
#define INT32_32ALN 5    // int (int32_t)
#define ULONG64_64ALN 6  // unsigned long (uint64_t)
#define LONG64_64ALN 7   // long (int64_t)
#define FLOAT32_32ALN 8  // float IEEE-754
#define DOUBLE64_64ALN 9 // double IEEE-754

// ----- C++ specialized type signature generation -----
#ifdef __cplusplus
#include <cstdint>
// Other type returns -1 by default.
// These structs down below may be used as a consensus Type tag:
// int consensusTypeID = TypeVal<T>::value;
template <typename T> struct TypeVal { static const int value{-1}; };
template <> struct TypeVal<std::uint8_t> {
  static const int value{UCHAR8_32ALN};
};
template <> struct TypeVal<std::int8_t> {
  static const int value{CHAR8_32ALN};
};
template <> struct TypeVal<std::uint16_t> {
  static const int value{USHORT16_32ALN};
};
template <> struct TypeVal<std::int16_t> {
  static const int value{SHORT16_32ALN};
};
template <> struct TypeVal<std::uint32_t> {
  static const int value{UINT32_32ALN};
};
template <> struct TypeVal<std::int32_t> {
  static const int value{INT32_32ALN};
};
template <> struct TypeVal<std::uint64_t> {
  static const int value{ULONG64_64ALN};
};
template <> struct TypeVal<std::int64_t> {
  static const int value{LONG64_64ALN};
};
template <> struct TypeVal<float> { static const int value{FLOAT32_32ALN}; };
template <> struct TypeVal<double> { static const int value{DOUBLE64_64ALN}; };

#else // #ifdef __cplusplus
// ----- C specialized pseudo-generic execution -----
#define DATAPTR32_32ALN 10 // T* -- DPU pointer type.
#define CODEPTR32_32ALN 11 // T(*F)()  -- Function pointer type.

#define MULTILINE(...)                                                         \
  do {                                                                         \
    __VA_ARGS__                                                                \
  } while (0)

#define DO_GENERIC(Ttag, Func, ...)                                            \
  MULTILINE(switch (Ttag) {                                                    \
    case UCHAR8_32ALN:                                                         \
      Func(unsigned char, __VA_ARGS__);                                        \
      break;                                                                   \
    case CHAR8_32ALN:                                                          \
      Func(char, __VA_ARGS__);                                                 \
      break;                                                                   \
    case USHORT16_32ALN:                                                       \
      Func(unsigned short, __VA_ARGS__);                                       \
      break;                                                                   \
    case SHORT16_32ALN:                                                        \
      Func(short, __VA_ARGS__);                                                \
      break;                                                                   \
    case UINT32_32ALN:                                                         \
      Func(uint32_t, __VA_ARGS__);                                             \
      break;                                                                   \
    case INT32_32ALN:                                                          \
      Func(int32_t, __VA_ARGS__);                                              \
      break;                                                                   \
    case ULONG64_64ALN:                                                        \
      Func(uint64_t, __VA_ARGS__);                                             \
      break;                                                                   \
    case LONG64_64ALN:                                                         \
      Func(int64_t, __VA_ARGS__);                                              \
      break;                                                                   \
    case FLOAT32_32ALN:                                                        \
      Func(float, __VA_ARGS__);                                                \
      break;                                                                   \
    case DOUBLE64_64ALN:                                                       \
      Func(double, __VA_ARGS__);                                               \
      break;                                                                   \
    default:                                                                   \
      break;                                                                   \
  }) // MULTILINE DO_GENERIC
     
// Those macros upside may be used as pseudo generic executor:
// A simple generic VA kernel might like this:
//
//#define KERNEL(T, count, src1, src2)                                         \
//  MULTILINE(                                                                 \
//    T *rhs1 = src1;                                                          \
//    T *rhs2 = src2;                                                          \
//    for (size_t i = 0; i != count; i++) {                                    \
//      rhs2[i] += rhs1[i];                                                    \
//     })
//#define EXEC_KERNEL(Ttag, ...) DO_GENERIC(Ttag, KERNEL, __VA_ARGS__)

#endif // #ifdef __cplusplus else

#endif // CONSENSUS_HPP
