#if !defined(DR_INTRINSICS_H)
#include "math.h"
#include "types.h"

internal i32 RoundReal32ToInt32(r32 Value) {
    i32 Result = (i32)roundf(Value);
    return (Result);
}

#define DR_INTRINSICS_H
#endif
