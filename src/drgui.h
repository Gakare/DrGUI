#if !defined(DRGUI_H)
#include "types.h"
#include "drgui_platform.h"

#if DR_INTERNAL
#define Assert(Expression)                          \
    if (!(Expression)) {                            \
        *(volatile int *)0 = 0;                     \
    }
#else
#define Assert(Expression)
#endif

#define ArrayCount(Arr) (sizeof(Arr) / sizeof((Arr)[0]))

#define Kilobytes(Val) ((Val) * 1024)
#define Megabytes(Val) (Kilobytes(Val) * 1024)
#define Gigabytes(Val) (Megabytes(Val) * 1024)
#define Terabytes(Val) (Gigabytes(Val) * 1024)

inline controller_input *GetController(input *Input, int ControllerIndex) {
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    controller_input *Result = &Input->Controllers[ControllerIndex];
    return (Result);
}

#define DRGUI_H
#endif
