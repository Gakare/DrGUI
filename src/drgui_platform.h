#if !defined(DRGUI_PLATFORM_H)

#include "types.h"

typedef struct button_state {
    int HalfTransitionCount;
    bool32 EndedDown;
} button_state;

typedef struct controller_input {
    bool32 IsConnected;
    bool32 IsAnalog;
    real32 StickAverageX;
    real32 StickAverageY;

    union {
        button_state Buttons[12];
        struct {
            button_state MoveUp;
            button_state MoveDown;
            button_state MoveLeft;
            button_state MoveRight;

            button_state ActionUp;
            button_state ActionDown;
            button_state ActionLeft;
            button_state ActionRight;

            button_state LeftShoulder;
            button_state RightShoulder;
            button_state Start;
            button_state Back;
        };
    };
} controller_input;

typedef struct input {
    // TODO: Deciding if it should just be 1 controller
    controller_input Controllers[5];
} input;

#define DRGUI_PLATFORM_H
#endif
