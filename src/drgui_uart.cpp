#include "drgui_uart.h"
#include "drgui_math.h"

// NOTE: This will be a manual packing of bytes into the buffer
internal void PacketSerialize(uint8_t *Buffer, drone_data Data) {
    if (Buffer) {
        Buffer[0] = (Data.LXInput >> 0) & 0xFF;
        Buffer[1] = (Data.LXInput >> 8) & 0xFF;
        Buffer[2] = (Data.LXInput >> 16) & 0xFF;
        Buffer[3] = (Data.LXInput >> 24) & 0xFF;

        Buffer[4] = (Data.LYInput >> 0) & 0xFF;
        Buffer[5] = (Data.LYInput >> 8) & 0xFF;
        Buffer[6] = (Data.LYInput >> 16) & 0xFF;
        Buffer[7] = (Data.LYInput >> 24) & 0xFF;
    }
}

internal drone_data PacketDeserialize(uint8_t *Buffer) {
    drone_data Result = {};
    if (Buffer) {
        Result.LXInput = (int)Buffer[0] | ((int)Buffer[1] << 8) | ((int)Buffer[2] << 16) |
                         ((int)Buffer[3] << 24);
        Result.LYInput = (int)Buffer[4] | ((int)Buffer[5] << 8) | ((int)Buffer[6] << 16) |
                         ((int)Buffer[7] << 24);
    }
    return (Result);
}

internal void UpdateAndCommunicate(platform_com_dev ComDev, drone_data DataPacket) {
    drone_data Data = {};
    Data.LXInput = 20;
    Data.LYInput = 40;

    uint8_t Writebuffer[8] = {};
    PacketSerialize(Writebuffer, Data);

    drone_data Data2 = {};

    uint8_t ReadBuffer[8] = {};
    int Index = 0;
    #if 0
    if (WaitCommEvent(ComDev.ComHandle, &EventMask, NULL)) {
        if (EventMask & EV_TXEMPTY) {
            if(!WriteFile(ComDev.ComHandle, Writebuffer, sizeof(Writebuffer),
                          &bytesWritten, NULL)) {
                // NOTE: Failed to send
            }
        }

        if (EventMask & EV_RXCHAR) {
            uint8_t Buffer;
            // NOTE: Reads 1 byte at a time
            while ((Index < 8) && ReadFile(ComDev.ComHandle, (void *)&Buffer,
                                           sizeof(Buffer), &bytesRead, NULL))
            {
                ReadBuffer[Index++] = Buffer;
            }
        }
    }
    #endif
    Data2 = PacketDeserialize(ReadBuffer);
}
