#if !defined(WIN32_DRGUI_H)
#include "drgui.h"
#include <windows.h>

struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension {
    int Width;
    int Height;
};

struct win32_state {
    u64 RenderMemoryTotalSize;
    void *RenderMemoryBlock;
};

#define WIN32_DRGUI_H
#endif
