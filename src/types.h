#if !defined(TYPES_H)
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef u32 b32;

typedef float  r32;
typedef double r64;

typedef size_t memory_index;

#define TYPES_H
#endif
