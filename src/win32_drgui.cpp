#include "drgui.cpp"
#include "drgui.h"
#include "drgui_platform.h"


/* Included libraries */
#include <WinSock2.h>
#include <ws2bth.h>
#include <bthsdpdef.h>
#include <bluetoothapis.h>

#include "win32_drgui.h"

#include <windows.h>
#include <Xinput.h>
#include <stdio.h>

/* Copied from Casey Muratori */
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return (ERROR_DEVICE_NOT_CONNECTED); }

global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

/* Global Variables */
global_variable b32 GlobalRunning = true;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};
global_variable b32 GlobalIsFullScreen;

internal void ToggleFullScreen(HWND Window) {
    // NOTE: This is from Raymond Chen's blog
    // https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY),
                           &MonitorInfo)) {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP, MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
                         SWP_FRAMECHANGED);
    }
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window) {
    win32_window_dimension Result = {};
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return (Result);
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) {
    if (Buffer->Memory) {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    // NOTE: This treats bitmaps as top-down, not bottom-up
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitMapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitMapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width * BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, win32_offscreen_buffer *Buffer,
                                         int WindowWidth, int WindowHeight) {
    if ((WindowWidth >= Buffer->Width * 2) && (WindowHeight >= Buffer->Height * 2)) {
        StretchDIBits(DeviceContext, 0, 0, 2 * Buffer->Width, 2 * Buffer->Height, 0, 0,
                      Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info,
                      DIB_RGB_COLORS, SRCCOPY);
    } else {
        int OffsetX = 10;
        int OffsetY = 10;

        PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
        PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight,
               BLACKNESS);
        PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight,
               BLACKNESS);

        // NOTE: Not stretching pixels, just 1:1 ratio
        StretchDIBits(DeviceContext, OffsetX, OffsetY, Buffer->Width, Buffer->Height, 0, 0,
                      Buffer->Width, Buffer->Height, Buffer->Memory, &Buffer->Info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
}

LRESULT CALLBACK Win32MainWindowCallBack(HWND Window, UINT Msg, WPARAM WParam, LPARAM LParam) {
    LRESULT Result = 0;
        switch (Msg) {
        case WM_SIZE: {
        } break;
        case SW_MAXIMIZE: {
        } break;

        case WM_SETCURSOR: {
        } break;

        case WM_DESTROY: {
            GlobalRunning = false;
        } break;

        case WM_CLOSE: {
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP: {
#if 0
            if (WParam == TRUE) {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255,
                                           LWA_ALPHA);
            } else {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 64, LWA_ALPHA);
            }
#endif
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
        } break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer,
                                       Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default: {
            Result = DefWindowProcA(Window, Msg, WParam, LParam);
        } break;
    }

    return (Result);
}

internal void Win32ProcessKeyboardMessage(button_state *NewState, b32 IsDown) {
    if (NewState->EndedDown != IsDown) {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void Win32LoadXInput(void) {
    HMODULE XInputLibrary = LoadLibrary("xinputuap.dll");
    // TODO: Maybe add more libraries, but should be good?
    if (!XInputLibrary) {
        XInputLibrary = LoadLibrary("xinput1_4.dll");
    }
    if (!XInputLibrary) {
        XInputLibrary = LoadLibrary("xinput9_1_0.dll");
    }
    if (!XInputLibrary) {
        XInputLibrary = LoadLibrary("xinput1_3.dll");
    }

    if (XInputLibrary) {
        XInputGetState = 
            (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState =
            (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

internal void Win32ProcessPendingMessages(controller_input *KeyboardController) {
    MSG Message;
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
        switch (Message.message) {
            case WM_QUIT: {
                GlobalRunning = false;
            } break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
                u32 VKCode = (u32)Message.wParam;
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                if (WasDown != IsDown) {
                    if (VKCode == VK_ESCAPE) {
                        GlobalRunning = false;
                    } else if (VKCode == VK_OEM_COMMA || VKCode == 'W') {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                    } else if (VKCode == 'O' || VKCode == 'S') {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    } else if (VKCode == 'E' || VKCode == 'D') {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    } else if (VKCode == 'A') {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }
                }

                if (IsDown) {
                    b32 AltKeyIsDown = (Message.lParam & (1 << 29));
                    if ((VKCode == VK_F4) && AltKeyIsDown) {
                        GlobalRunning = false;
                    }

                    if ((VKCode == VK_RETURN) && AltKeyIsDown) {
                        if (Message.hwnd) {
                            ToggleFullScreen(Message.hwnd);
                        }
                    }
                }
            } break;
            default: {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;;
        }
    }
}

// TODO: ENSURE DEADZONE
internal r32 Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold) {
    r32 Result = 0;
    if (Value < -DeadZoneThreshold) {
        Result = (r32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
    } else if (Value > DeadZoneThreshold) {
        Result = (r32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
    }
    return (Result);
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
                                              button_state *OldState,
                                              button_state *NewState,
                                              DWORD ButtonBit) {
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine,
                     int ShowCmd) {
    win32_state Win32State = {};

    Win32LoadXInput();

    WNDCLASSA WindowClass = {};

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallBack;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "DrGUIWindowClass";
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);

    // TODO: Determine which one is the better default
    Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);
    //Win32ResizeDIBSection(&GlobalBackBuffer, 1920, 1080);

    if (RegisterClassA(&WindowClass)) {
        HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "DrGUI",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
        if (Window) {
#if !DR_INTERNAL
            if (!GlobalIsFullScreen) {
                ToggleFullScreen(Window);
            }
#endif
            // HDC RefreshDC = GetDC(Window);

            GlobalRunning = true;

#if DR_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes((u64)2);
#else
            LPVOID BaseAddress = 0;
#endif

            // TODO: This is how I will connect to the HC-05
            LPCWSTR PortName = L"\\\\.\\COM4";
            HANDLE ComHandle = CreateFileW(PortName,
                                           GENERIC_READ | GENERIC_WRITE, 0,
                                           NULL, OPEN_EXISTING, 0, NULL);
            if (ComHandle == INVALID_HANDLE_VALUE) {
#if DR_INTERNAL
                OutputDebugStringA("Failed to open COM");
#endif
            }

            DCB Win32DCB = {};
            Win32DCB.DCBlength = sizeof(Win32DCB);
            GetCommState(ComHandle, &Win32DCB);
            Win32DCB.BaudRate = 9600;
            Win32DCB.ByteSize = 8;
            Win32DCB.Parity = NOPARITY;
            Win32DCB.StopBits = ONESTOPBIT;
            SetCommState(ComHandle, &Win32DCB);

            char buffer[256];
            //DWORD bytesWritten;
            DWORD bytesRead;

             // Write data
            //WriteFile(ComHandle, "Hello", 6, &bytesWritten, NULL);

            COMMTIMEOUTS Win32Timeout = {};
            Win32Timeout.ReadIntervalTimeout = 10;
            Win32Timeout.ReadTotalTimeoutMultiplier = 10;
            Win32Timeout.ReadTotalTimeoutConstant = 1000;
            SetCommTimeouts(ComHandle, &Win32Timeout);
            // Read data
            if (ReadFile(ComHandle, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
                buffer[bytesRead] = '\0';
                OutputDebugStringA(buffer);
            }

            // TODO: Omit this, ONLY FOR DEBUGGING
            CloseHandle(ComHandle);

            render_memory RenderMemory = {};
            RenderMemory.PermanentStorageSize = Megabytes(64);
            RenderMemory.TransientStorageSize = Gigabytes(1);
            Win32State.TotalSize =
                RenderMemory.PermanentStorageSize + RenderMemory.TransientStorageSize;
            Win32State.RenderMemoryBlock =
                VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
                             MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            RenderMemory.PermanentStorage = Win32State.RenderMemoryBlock;
            RenderMemory.TransientStorage =
                ((u8 *)RenderMemory.PermanentStorage + RenderMemory.PermanentStorageSize);

            if (RenderMemory.PermanentStorage && RenderMemory.TransientStorage) {
                input Input[2] = {};
                input *NewInput = &Input[0];
                input *OldInput = &Input[1];

                while (GlobalRunning) {
                    controller_input *OldKeyboardController = GetController(OldInput, 0);
                    controller_input *NewKeyboardController = GetController(NewInput, 0);
                    *NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;
                    for (int ButtonIndex = 0;
                    ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                    ++ButtonIndex) {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(NewKeyboardController);

                    // TODO: Deciding if it should only be 1 controller
                    DWORD MaxControllerCount = XUSER_MAX_COUNT;
                    if (MaxControllerCount >
                        (ArrayCount(NewInput->Controllers) - 1)) {
                        MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
                    }

                    // TODO: Should this be polled more frequently?
                    for (DWORD ControllerIndex = 0;
                    ControllerIndex < MaxControllerCount;
                    ++ControllerIndex) {
                        DWORD OurControllerIndex = ControllerIndex + 1;
                        controller_input *OldController =
                            GetController(OldInput, OurControllerIndex);
                        controller_input *NewController =
                            GetController(NewInput, OurControllerIndex);

                        XINPUT_STATE ControllerState;
                        if (XInputGetState(ControllerIndex, &ControllerState) ==
                            ERROR_SUCCESS) {
                            NewController->IsConnected = true;
                            NewController->IsAnalog = OldController->IsAnalog;

                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                            NewController->LStickAverageX = Win32ProcessXInputStickValue(
                                Pad->sThumbLX,
                                XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            NewController->LStickAverageY = Win32ProcessXInputStickValue(
                                Pad->sThumbLY,
                                XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                            NewController->RStickAverageX = Win32ProcessXInputStickValue(
                                Pad->sThumbRX,
                                XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
                            NewController->RStickAverageY = Win32ProcessXInputStickValue(
                                Pad->sThumbRY,
                                XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

                            if ((NewController->LStickAverageX != 0.0f) ||
                                (NewController->LStickAverageY != 0.0f) ||
                                (NewController->RStickAverageX != 0.0f) ||
                                (NewController->RStickAverageY != 0.0f)) {
                                NewController->IsAnalog = true;
                            }

                            // TODO: I might want to use the dpad for something else,
                            // can't be for movement for the copter
#if 0
                            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
                                NewController->StickAverageY = 1.0f;
                            }
                            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
                                NewController->StickAverageY = -1.0f;
                            }
                            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
                                NewController->StickAverageX = -1.0f;
                            }
                            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
                                NewController->StickAverageX = 1.0f;
                            }
#endif

                            r32 Threshold = 0.5f;
                            Win32ProcessXInputDigitalButton(
                                (NewController->LStickAverageX < -Threshold) ? 1 : 0,
                                &OldController->MoveLeft, &NewController->MoveLeft, 1);
                            Win32ProcessXInputDigitalButton(
                                (NewController->LStickAverageX > Threshold) ? 1 : 0,
                                &OldController->MoveRight, &NewController->MoveRight, 1);
                            Win32ProcessXInputDigitalButton(
                                (NewController->LStickAverageY < -Threshold) ? 1 : 0,
                                &OldController->MoveDown, &NewController->MoveDown, 1);
                            Win32ProcessXInputDigitalButton(
                                (NewController->LStickAverageY > Threshold) ? 1 : 0,
                                &OldController->MoveUp, &NewController->MoveUp, 1);

                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons, &OldController->ActionDown,
                                &NewController->ActionDown, XINPUT_GAMEPAD_A);
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons, &OldController->ActionRight,
                                &NewController->ActionRight, XINPUT_GAMEPAD_B);
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons, &OldController->ActionLeft,
                                &NewController->ActionLeft, XINPUT_GAMEPAD_X);
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons, &OldController->ActionUp,
                                &NewController->ActionUp, XINPUT_GAMEPAD_Y);
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons, &OldController->LeftShoulder,
                                &NewController->LeftShoulder,
                                XINPUT_GAMEPAD_LEFT_SHOULDER);
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons, &OldController->RightShoulder,
                                &NewController->RightShoulder,
                                XINPUT_GAMEPAD_RIGHT_SHOULDER);
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons, &OldController->Start,
                                &NewController->Start, XINPUT_GAMEPAD_START);
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons, &OldController->Back,
                                &NewController->Back, XINPUT_GAMEPAD_BACK);
                        } else {
                            // NOTE: The controller is not available
                            NewController->IsConnected = false;
                        }
                    }

                    offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackBuffer.Memory;
                    Buffer.Width = GlobalBackBuffer.Width;
                    Buffer.Height = GlobalBackBuffer.Height;
                    Buffer.Pitch = GlobalBackBuffer.Pitch;
                    Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;

                    UpdateAndRender(&Buffer, &RenderMemory, NewInput);

                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    HDC DeviceContext = GetDC(Window);
                    Win32DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer,
                                               Dimension.Width, Dimension.Height);
                    ReleaseDC(Window, DeviceContext);
                }

            } else {
                // TODO: Logging
            }

            // NOTE: This is where it will be closed
            CloseHandle(ComHandle);
        } else {
            // TODO: Logging
        }
    } else {
        // TODO: Logging
    }
}
