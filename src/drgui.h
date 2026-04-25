#if !defined(DRGUI_H)
/* TODO:
 *  COM Port
 *  - NOTE: VCC Use a constant 3.6V-6V for a stable power supply, mandatory.
 *  - NOTE: TX (PD1) - RX (PD0) on the Atmega328P/PB
 *
 *  Controller:
 *  - See if I can fix the controller sensitivity that will be used to control drone and send via UART.
 *
 *  GUI:
 *  - Display the inputs in a more controlled manner, ideally as close to what the joysticks are 
 *  doing in real life as possible.
 *  - Add graph data reading from the bluetooth. (This might be where I use multithreading on the GUI)
 *
 *  Multithreading:
 *  - Need to implement 2 threads
 *      - GUI thread
 *      - Communication thread
 *
*/

#include "types.h"
#include "drgui_platform.h"

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

struct gui_state {
    int XOffset[2];
    int YOffset[2];
};

internal void UpdateAndRender(offscreen_buffer *Buffer, gui_memory *Memory, input *Input);

global_variable platform_add_entry *PlatformAddEntry;
global_variable platform_complete_all_work *PlatformCompleteAllWork;

#define DRGUI_H
#endif
