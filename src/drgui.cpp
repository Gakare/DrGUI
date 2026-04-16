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

internal void UpdateAndRender(offscreen_buffer *Buffer, render_memory *Memory, input *Input) {

    Assert((&Input->Controllers[0].Back - &Input->Controllers[0].Buttons[0]) ==
           (ArrayCount(Input->Controllers[0].Buttons) - 1));

    Assert(sizeof(render_state) <= Memory->PermanentStorageSize);
    render_state *State = (render_state *)Memory->PermanentStorage;

    if (!Memory->IsInitialized) {
        State->XOffset = 0;
        State->YOffset = 0;

        Memory->IsInitialized = true;
    }

    for (int ControllerIndex = 0;
    ControllerIndex < ArrayCount(Input->Controllers);
    ++ControllerIndex) {
        controller_input *Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog) {
            // TODO: Figure out why my XOffset is off
            State->XOffset += (int)(Controller->StickAverageX);
            // TODO: Up and down is currently reversed
            State->YOffset -= (int)(Controller->StickAverageY);
        } else {
            // NOTE: This is keyboard
            if (Controller->MoveUp.EndedDown) {
                State->YOffset -= 10;
            }
            if (Controller->MoveDown.EndedDown) {
                State->YOffset += 10;
            }
            if (Controller->MoveRight.EndedDown) {
                State->XOffset += 10;
            }
            if (Controller->MoveLeft.EndedDown) {
                State->XOffset -= 10;
            }
        }

        if (Controller->ActionDown.EndedDown){
            --State->YOffset;
        }
        if (Controller->ActionUp.EndedDown) {
            ++State->YOffset;
        }
        if (Controller->ActionRight.EndedDown) {
            ++State->XOffset;
        }
        if (Controller->ActionLeft.EndedDown) {
            --State->XOffset;
        }
    }

    r32 ScreenMidPointX = (r32)Buffer->Width * 0.5f;
    r32 ScreenMidPointY = (r32)Buffer->Height * 0.5f;

    // NOTE: Clearing Screen
    DrawRectangle(Buffer, 0, 0, (r32)Buffer->Width, (r32)Buffer->Height, 0xFF555555);

    // NOTE: LeftStick
    r32 StickBoundOffset = 100.0f;
    r32 StickBoundMinX = ScreenMidPointX - (ScreenMidPointX * 0.5f) - StickBoundOffset;
    r32 StickBoundMinY = ScreenMidPointY + (ScreenMidPointY * 0.5f) - StickBoundOffset;
    r32 StickBoundMaxX = ScreenMidPointX - (ScreenMidPointX * 0.5f) + StickBoundOffset;
    r32 StickBoundMaxY = ScreenMidPointY + (ScreenMidPointY * 0.5f) + StickBoundOffset;
    DrawRectangle(Buffer, StickBoundMinX, StickBoundMinY,
                  StickBoundMaxX, StickBoundMaxY, 0xFFFF0000);

    r32 StickOffset = 8.0f;
    DrawRectangle(Buffer, StickBoundMinX + StickOffset, StickBoundMinY + StickOffset,
                  StickBoundMaxX - StickOffset, StickBoundMaxY - StickOffset, 0xFF000000);

    r32 ControllerOffset = 50.0f;
    DrawRectangle(Buffer, StickBoundMinX + ControllerOffset + State->XOffset ,
                  StickBoundMinY + ControllerOffset + State->YOffset,
                  StickBoundMaxX - ControllerOffset + State->XOffset, 
                  StickBoundMaxY - ControllerOffset + State->YOffset, 0xFF999999);


    // NOTE: Buttons
    DrawRectangle(Buffer, (ScreenMidPointX + (ScreenMidPointX * 0.5f)) - 100.0f,
                  (ScreenMidPointY * 0.5f) - 100.0f,
                 (ScreenMidPointX + (ScreenMidPointX * 0.5f)) + 100.0f,
                  (ScreenMidPointY * 0.5f) + 100.0f, 0xFFFF0000);

    // NOTE: Debugging line coordinates
    DrawRectangle(Buffer, 0, ScreenMidPointY, (r32)Buffer->Width, ScreenMidPointY + 1.0f, 0xFFFFFF00);
    DrawRectangle(Buffer, ScreenMidPointX - 1.0f, 0, ScreenMidPointX + 1.0f, 
                  (r32)Buffer->Height, 0xFFFFFF00);
}
