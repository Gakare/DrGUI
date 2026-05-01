#if !defined(DRGUI_UART_H)
#include "drgui_platform.h"

enum uart_event_type {
    UARTType_Rx,
    UARTType_Tx,
};

struct uart_event {
    uart_event_type Type;
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

internal u32 PacketSerialize(u8 *Buffer, drone_data Data);
internal drone_data PacketDeserialize(u8 *Buffer);
internal drone_data ProcessInputForSendPacket(input *Input);

#define DRGUI_UART_H 
#endif
