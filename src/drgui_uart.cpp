#include "drgui_uart.h"
#include "drgui_math.h"

// NOTE: This will be a manual packing of bytes into the buffer
internal u32 PacketSerialize(u8 *Buffer, drone_data Data) {
    u32 Result = 0;
    if (Buffer) {
        Buffer[0] = (Data.LXInput >> 0) & 0xFF;
        Buffer[1] = (Data.LXInput >> 8) & 0xFF;
        Buffer[2] = (Data.LXInput >> 16) & 0xFF;
        Buffer[3] = (Data.LXInput >> 24) & 0xFF;

        Buffer[4] = (Data.LYInput >> 0) & 0xFF;
        Buffer[5] = (Data.LYInput >> 8) & 0xFF;
        Buffer[6] = (Data.LYInput >> 16) & 0xFF;
        Buffer[7] = (Data.LYInput >> 24) & 0xFF;
        
        Result = 8;
    }
    return (Result);
}

internal drone_data PacketDeserialize(u8 *Buffer) {
    drone_data Result = {};
    if (Buffer) {
        Result.LXInput = (int)Buffer[0] | ((int)Buffer[1] << 8) | ((int)Buffer[2] << 16) |
                         ((int)Buffer[3] << 24);
        Result.LYInput = (int)Buffer[4] | ((int)Buffer[5] << 8) | ((int)Buffer[6] << 16) |
                         ((int)Buffer[7] << 24);
    }
    return (Result);
}
