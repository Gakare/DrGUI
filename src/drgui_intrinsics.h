#if !defined(DR_INTRINSICS_H)
#include "math.h"
#include "types.h"

inline i32 RoundReal32ToInt32(r32 Value) {
    i32 Result = (i32)roundf(Value);
    return (Result);
}

inline r32 Sqrt(r32 Value) {
    r32 Result = sqrtf(Value);
    return (Result);
}


#define DR_INTRINSICS_H
#endif
