#if !defined(DRGUI_H)
/* TODO:
 *  COM Port
 *  - Connect to the COM PORT (Currently Com 4). Will need to adjust if connecting with another device
 *  - Be able to Write data
 *  - Be able to Read data
 *  - VCC: Use a constant 3.6V-6V for a stable power supply, mandatory.
 *  - TX (PD1) - RX (PD0) 
 *
 *  Controller:
 *  - See if I can fix the controller sensitivity that will be used to control drone and send via UART.
 *
 *  GUI:
 *  - Display the inputs in a more controlled manner, ideally as close to what the joysticks are 
 *  doing in real life as possible.
 *  - Add graph data reading from the bluetooth.
 *
 *  Multithreading:
 *  - Need to implement 3 threads
 *      - Main thread
 *      - Write thread
 *      - Read thread
 *  - I assume the only race condition that I should worry about is when the write thread and read
 *    thread need to access the com handle at the same time. (Can't wait to be wrong)
 *
*/
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

#pragma pack(push, 1)
// NOTE: Create a serialization and deserialization protocol
// - Left stick XInput
// - Left stick YInput
// - Roll
// - Pitch
// - Yaw
struct drone_data {
    int LXInput;
    int LYInput;
};
#pragma pack(pop)

internal void UpdateAndRender(offscreen_buffer *Buffer, render_memory *Memory, input *Input);

#define DRGUI_H
#endif
