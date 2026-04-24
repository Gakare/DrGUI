#include "drgui.h"
#include "drgui_math.h"

internal void DrawRectangle(offscreen_buffer *Buffer, r32 Minx, r32 Miny, r32 Maxx, r32 Maxy, 
                            u32 Color = 0) {
    i32 MinX = RoundReal32ToInt32(Minx);
    i32 MinY = RoundReal32ToInt32(Miny);
    i32 MaxX = RoundReal32ToInt32(Maxx);
    i32 MaxY = RoundReal32ToInt32(Maxy);

    if (MinX < 0) {
        MinX = 0;
    }
    if (MinY < 0) {
        MinY =  0;
    }

    if (MaxX > Buffer->Width) {
        MaxX = Buffer->Width;
    }
    if (MaxY > Buffer->Height) {
        MaxY = Buffer->Height;
    }

    u8 *Row = ((u8 *)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
    for (int Y = MinY; Y < MaxY; ++Y) {
        u32 *Pixel = (u32 *)Row;
        for (int X = MinX; X < MaxX; ++X) {
            *Pixel++ = Color;
        }
        Row += Buffer->Pitch;
    }
}

internal void UpdateAndRender(offscreen_buffer *Buffer, gui_memory *Memory, input *Input) {

    Assert((&Input->Controllers[0].Back - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons) - 1));

    Assert(sizeof(gui_state) <= Memory->PermanentStorageSize);
    gui_state *State = (gui_state  *)Memory->PermanentStorage;

    if (!Memory->IsInitialized) {
        PlatformAddEntry = Memory->PlatformAddEntry;
        PlatformCompleteAllWork = Memory->PlatformCompleteAllWork;

        State->XOffset[0] = 0;
        State->YOffset[0] = 0;
        State->XOffset[1] = 0;
        State->YOffset[1] = 0;

        Memory->IsInitialized = true;
    }

    for (int ControllerIndex = 0;
    ControllerIndex < ArrayCount(Input->Controllers);
    ++ControllerIndex) {
        controller_input *Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog) {
            // TODO: Figure out why my XOffset is off
            State->XOffset[0] += (int)(Controller->LStickAverageX);
            // TODO: Up and down is currently reversed, do I want to keep it that way?
            State->YOffset[0] -= (int)(Controller->LStickAverageY);

            State->XOffset[1] += (int)(Controller->RStickAverageX);
            State->YOffset[1] -= (int)(Controller->RStickAverageY);
        } else {
            // NOTE: This is keyboard
            if (Controller->MoveUp.EndedDown) {
                State->YOffset[0] -= 10;
            }
            if (Controller->MoveDown.EndedDown) {
                State->YOffset[0] += 10;
            }
            if (Controller->MoveRight.EndedDown) {
                State->XOffset[0] += 10;
            }
            if (Controller->MoveLeft.EndedDown) {
                State->XOffset[0] -= 10;
            }
        }

        if (Controller->ActionDown.EndedDown){
            --State->YOffset[0];
        }
        if (Controller->ActionUp.EndedDown) {
            ++State->YOffset[0];
        }
        if (Controller->ActionRight.EndedDown) {
            ++State->XOffset[0];
        }
        if (Controller->ActionLeft.EndedDown) {
            --State->XOffset[0];
        }
    }

    r32 ScreenMidPointX = (r32)Buffer->Width * 0.5f;
    r32 ScreenMidPointY = (r32)Buffer->Height * 0.5f;

    // NOTE: Clearing Screen / Background
    DrawRectangle(Buffer, 0, 0, (r32)Buffer->Width, ScreenMidPointY, 0xFF222222);
    DrawRectangle(Buffer, 0, ScreenMidPointY, (r32)Buffer->Width, (r32)Buffer->Height, 0xFF555555);

    // NOTE: LeftStick
    r32 LeftBoundOffset = 100.0f;
    r32 LeftBoundMinX = ScreenMidPointX - (ScreenMidPointX * 0.5f) - LeftBoundOffset;
    r32 LeftBoundMinY = ScreenMidPointY + (ScreenMidPointY * 0.5f) - LeftBoundOffset;
    r32 LeftBoundMaxX = ScreenMidPointX - (ScreenMidPointX * 0.5f) + LeftBoundOffset;
    r32 LeftBoundMaxY = ScreenMidPointY + (ScreenMidPointY * 0.5f) + LeftBoundOffset;
    DrawRectangle(Buffer, LeftBoundMinX, LeftBoundMinY,
                  LeftBoundMaxX, LeftBoundMaxY, 0xFFFF0000);

    r32 StickOffset = 8.0f;
    DrawRectangle(Buffer, LeftBoundMinX + StickOffset, LeftBoundMinY + StickOffset,
                  LeftBoundMaxX - StickOffset, LeftBoundMaxY - StickOffset, 0xFF000000);

    r32 ControllerOffset = 50.0f;
    // TODO: Add bounds check on the left stick display
    r32 LControllerMinX = LeftBoundMinX + ControllerOffset + State->XOffset[0];
    r32 LControllerMinY = LeftBoundMinY + ControllerOffset + State->YOffset[0];
    r32 LControllerMaxX = LeftBoundMaxX - ControllerOffset + State->XOffset[0];
    r32 LControllerMaxY = LeftBoundMaxY - ControllerOffset + State->YOffset[0];
    DrawRectangle(Buffer, LControllerMinX, LControllerMinY, LControllerMaxX,
                  LControllerMaxY, 0xFF999999); 

    // NOTE: Buttons
    r32 RightBoundOffset = 100.0f;
    r32 RightBoundMinX = ScreenMidPointX + (ScreenMidPointX * 0.5f) - RightBoundOffset;
    r32 RightBoundMinY = ScreenMidPointY + (ScreenMidPointY * 0.5f) - RightBoundOffset;
    r32 RightBoundMaxX = ScreenMidPointX + (ScreenMidPointX * 0.5f) + RightBoundOffset;
    r32 RightBoundMaxY = ScreenMidPointY + (ScreenMidPointY * 0.5f) + RightBoundOffset;
    DrawRectangle(Buffer, RightBoundMinX, RightBoundMinY, RightBoundMaxX,
                  RightBoundMaxY, 0xFFFF0000);

    r32 RBlackMinX = RightBoundMinX + StickOffset;
    r32 RBlackMinY = RightBoundMinY + StickOffset;
    r32 RBlackMaxX =  RightBoundMaxX - StickOffset;
    r32 RBlackMaxY = RightBoundMaxY - StickOffset;
    DrawRectangle(Buffer, RBlackMinX, RBlackMinY, RBlackMaxX, RBlackMaxY, 0xFF000000);

    r32 RControllerMinX = RightBoundMinX + ControllerOffset + State->XOffset[1];
    r32 RControllerMinY = RightBoundMinY + ControllerOffset + State->YOffset[1];
    r32 RControllerMaxX = RightBoundMaxX - ControllerOffset + State->XOffset[1];
    r32 RControllerMaxY = RightBoundMaxY - ControllerOffset + State->YOffset[1];

    if (RControllerMinX > RBlackMaxX) {
        r32 XDiff = RControllerMaxX - RControllerMinX;
        RControllerMinX = RBlackMaxX;
        RControllerMaxX = RControllerMinX + XDiff;
    }
    if (RControllerMinY > RBlackMaxY) {
        r32 YDiff = RControllerMaxY - RControllerMinY;
        RControllerMinY = RBlackMaxY;
        RControllerMaxY = RControllerMinY + YDiff;
    }
    if (RControllerMaxX < RBlackMinX) {
        r32 XDiff = RControllerMaxX - RControllerMinX;
        RControllerMaxX = RBlackMinX;
        RControllerMinX = RControllerMaxX - XDiff;
    }
    if (RControllerMaxY < RBlackMinY) {
        r32 YDiff = RControllerMaxY - RControllerMinY;
        RControllerMaxY = RBlackMinY;
        RControllerMinY = RControllerMaxY - YDiff;
    }

    DrawRectangle(Buffer, RControllerMinX, RControllerMinY, RControllerMaxX,
                  RControllerMaxY, 0xFF999999); 

    // NOTE: Debugging line coordinates
    // DrawRectangle(Buffer, 0, ScreenMidPointY, (r32)Buffer->Width, ScreenMidPointY + 1.0f, 0xFFFFFF00);
    // DrawRectangle(Buffer, ScreenMidPointX - 1.0f, 0, ScreenMidPointX + 1.0f, 
    //               (r32)Buffer->Height, 0xFFFFFF00);
}
