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

struct memory_arena {
    memory_index Size;
    u8 *Base;
    memory_index Used;

    u32 TempCount;
};

struct temporary_memory {
    memory_arena *Arena;
    memory_index Used;
};

struct render_state {
    int XOffset[2];
    int YOffset[2];
};

internal void UpdateAndRender(offscreen_buffer *Buffer, render_memory *Memory, input *Input);

#define DRGUI_H
#endif
