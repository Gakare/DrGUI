#if !defined(DRGUI_UART_H)
#include "drgui_platform.h"

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

internal void UpdateAndCommunicate(platform_com_dev ComDev, drone_data DataPacket);

#define DRGUI_UART_H 
#endif
