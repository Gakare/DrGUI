#include "drgui.cpp"
#include "drgui.h"
#include "drgui_platform.h"
#include "win32_drgui.h"

/* Included libraries */
#include <windows.h>
#include <intrin.h>
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

internal HANDLE Win32ConnectComDevice(LPCWSTR PortName) {
    HANDLE Result;
    Result = CreateFileW(PortName, 
                         GENERIC_READ | GENERIC_WRITE, 0,
                         NULL, OPEN_EXISTING, 0, NULL);
    return (Result);
}

internal void Win32ConfigureComDevice(HANDLE Win32ComHandle) {
    DCB Win32DCB = {};
    Win32DCB.DCBlength = sizeof(Win32DCB);
    GetCommState(Win32ComHandle, &Win32DCB);
    Win32DCB.BaudRate = 9600;
    Win32DCB.ByteSize = 8;
    Win32DCB.Parity = NOPARITY;
    Win32DCB.StopBits = ONESTOPBIT;
    SetCommState(Win32ComHandle, &Win32DCB);

    /* NOTE: RTmax = (N (amount in bytes) * ReadTotalTimeoutM) + ReadTotalTimoutConst
     *       WTmax = (N (amount in bytes) * WriteTotalTimoutM) + WriteTotalTimoutConst
     */
    COMMTIMEOUTS Win32Timeout = {};
    Win32Timeout.ReadTotalTimeoutMultiplier = 0;
    Win32Timeout.ReadTotalTimeoutConstant = 0;
    Win32Timeout.ReadIntervalTimeout = 0;
    Win32Timeout.WriteTotalTimeoutMultiplier = 0;
    Win32Timeout.WriteTotalTimeoutConstant = 0;
    SetCommTimeouts(Win32ComHandle, &Win32Timeout);

    // NOTE: Monitors receiving characters and transmission completion
    SetCommMask(Win32ComHandle, EV_RXCHAR | EV_TXEMPTY);
}

// NOTE: This will be a manual packing of bytes into the buffer
internal void Win32Serialize(uint8_t *Buffer, drone_data Data) {
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

internal drone_data Win32Deserialize(uint8_t *Buffer) {
    drone_data Result = {};
    if (Buffer) {
        Result.LXInput = (int)Buffer[0] | ((int)Buffer[1] << 8) | ((int)Buffer[2] << 16) |
                         ((int)Buffer[3] << 24);
        Result.LYInput = (int)Buffer[4] | ((int)Buffer[5] << 8) | ((int)Buffer[6] << 16) |
                         ((int)Buffer[7] << 24);
    }
    return (Result);
}

struct platform_work_queue_entry {
    platform_work_queue_callback *Callback;
    void *Data;
};

struct platform_work_queue {
    u32 volatile CompletionGoal;
    u32 volatile CompletionCount;

    u32 volatile NextEntryToWrite;
    u32 volatile NextEntryToRead;
    HANDLE SemaphoreHandle; 

    platform_work_queue_entry Entries[256];
};

internal void Win32AddEntry(platform_work_queue *Queue, platform_work_queue_callback *Callback,
                            void *Data) {
    u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
    Assert(NewNextEntryToWrite != Queue->NextEntryToRead);
    platform_work_queue_entry *Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Callback = Callback;
    Entry->Data = Data;
    ++Queue->CompletionGoal;
    _WriteBarrier();
    _mm_sfence();
    Queue->NextEntryToWrite = NewNextEntryToWrite;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal b32 Win32DoNextWorkQueueEntry(platform_work_queue *Queue) {
    b32 ShouldSleep = false;

    u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
    if (OriginalNextEntryToRead != Queue->NextEntryToWrite) {
        u32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                               NewNextEntryToRead, OriginalNextEntryToRead);

        // NOTE: This is for spmc, but its fine for spsc
        if (Index == OriginalNextEntryToRead) {
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Queue, Entry.Data);
            InterlockedIncrement((LONG volatile *)&Queue->CompletionCount);
        }
    } else {
        ShouldSleep = true;
    }

    return (ShouldSleep);
}

internal void Win32CompleteAllWork(platform_work_queue *Queue) {
    while (Queue->CompletionCount != Queue->CompletionGoal) {
        Win32DoNextWorkQueueEntry(Queue);
    }

    Queue->CompletionCount = 0;
    Queue->CompletionGoal = 0;
}

struct win32_thread_info {
    int LogicalThreadIndex;
    platform_work_queue *Queue;
};

// TODO: This will be where the communication thread handles either Read or Write
DWORD WINAPI ThreadProc(LPVOID lpParameter) {
    win32_thread_info *ThreadInfo = (win32_thread_info *)lpParameter;

    for (;;) {
        if (Win32DoNextWorkQueueEntry(ThreadInfo->Queue)) {
            WaitForSingleObjectEx(ThreadInfo->Queue->SemaphoreHandle, INFINITE, FALSE);
        }
    }

    //return (0);
}

internal PLATFORM_WORK_QUEUE_CALLBACK(DoWorkerWork) {
    char Buffer[256];
    wsprintf(Buffer, "Thread %u: %s\n", GetCurrentThreadId(), (char *)Data);
    OutputDebugStringA(Buffer);
}


int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine,
                     int ShowCmd) {
    win32_state Win32State = {};

    win32_thread_info ThreadInfo[1] = {};

    platform_work_queue Queue = {};

    u32 InitialCount = 0;
    u32 ThreadCount = ArrayCount(ThreadInfo);
    Queue.SemaphoreHandle = CreateSemaphoreExA(0, InitialCount, ThreadCount, 0, 0,
                                                SEMAPHORE_ALL_ACCESS);
    
    // NOTE: This creates only 1 extra thread which will handle Reads and Writes.
    for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex) {
        win32_thread_info *Info = ThreadInfo + ThreadIndex;
        Info->Queue = &Queue;
        Info->LogicalThreadIndex = ThreadIndex;

        DWORD ThreadID;
        HANDLE ThreadHandle = CreateThread(0, 0, ThreadProc, Info, 0, &ThreadID);
        CloseHandle(ThreadHandle);
    }

    Win32AddEntry(&Queue, DoWorkerWork, "String A0");
    Win32AddEntry(&Queue, DoWorkerWork, "String A1");
    Win32AddEntry(&Queue, DoWorkerWork, "String A2");
    Win32AddEntry(&Queue, DoWorkerWork, "String A3");
    Win32AddEntry(&Queue, DoWorkerWork, "String A4");
    Win32AddEntry(&Queue, DoWorkerWork, "String A5");
    Win32AddEntry(&Queue, DoWorkerWork, "String A6");
    Win32AddEntry(&Queue, DoWorkerWork, "String A7");
    Win32AddEntry(&Queue, DoWorkerWork, "String A8");
    Win32AddEntry(&Queue, DoWorkerWork, "String A9");

    Sleep(1000);

    Win32AddEntry(&Queue, DoWorkerWork, "String B0");
    Win32AddEntry(&Queue, DoWorkerWork, "String B1");
    Win32AddEntry(&Queue, DoWorkerWork, "String B2");
    Win32AddEntry(&Queue, DoWorkerWork, "String B3");
    Win32AddEntry(&Queue, DoWorkerWork, "String B4");
    Win32AddEntry(&Queue, DoWorkerWork, "String B5");
    Win32AddEntry(&Queue, DoWorkerWork, "String B6");
    Win32AddEntry(&Queue, DoWorkerWork, "String B7");
    Win32AddEntry(&Queue, DoWorkerWork, "String B8");
    Win32AddEntry(&Queue, DoWorkerWork, "String B9");

    Win32CompleteAllWork(&Queue);

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


#if DR_INTERNAL
            LPCWSTR PortName = L"\\\\.\\COM4";
#else
            LPCWSTR PortName = L"\\\\.\\COM6";
#endif

            HANDLE Win32ComHandle = Win32ConnectComDevice(PortName);
            if (Win32ComHandle != INVALID_HANDLE_VALUE) {
                Win32ConfigureComDevice(Win32ComHandle);

                // NOTE: Keeping this rn since this worked in a single thread.
#if 0
                DWORD bytesRead = 0;
                DWORD bytesWritten = 0;

                drone_data Data = {};
                Data.LXInput = 20;
                Data.LYInput = 40;

                uint8_t Writebuffer[8] = {};
                Win32Serialize(Writebuffer, Data);

                // TODO: Use this to see if I can receive the data from mcu
                drone_data Data2 = {};

                uint8_t ReadBuffer[8] = {};
                DWORD EventMask;
                int Index = 0;
                while (GlobalRunning) {
                    if (WaitCommEvent(Win32ComHandle, &EventMask, NULL)) {
                        if (EventMask & EV_TXEMPTY) {
                            if(!WriteFile(Win32ComHandle, Writebuffer, sizeof(Writebuffer),
                                          &bytesWritten, NULL)) {
                                // NOTE: Failed to send
                            }
                        }

                        if (EventMask & EV_RXCHAR) {
                            uint8_t Buffer;
                            // NOTE: Reads 1 byte at a time
                            while ((Index < 8) && ReadFile(Win32ComHandle, (void *)&Buffer,
                                                           sizeof(Buffer), &bytesRead, NULL) )
                            {
                                ReadBuffer[Index++] = Buffer;
                            }
                        }
                    }
                    Data2 = Win32Deserialize(ReadBuffer);
                }
#endif

                gui_memory GUIMemory = {};
                GUIMemory.PermanentStorageSize = Megabytes(128);
                GUIMemory.TransientStorageSize = Gigabytes(1);
                GUIMemory.HighPriorityQueue = &Queue;
                GUIMemory.PlatformAddEntry = Win32AddEntry;
                GUIMemory.PlatformCompleteAllWork = Win32CompleteAllWork;
                Win32State.TotalSize =
                    GUIMemory.PermanentStorageSize + GUIMemory.TransientStorageSize;
                Win32State.RenderMemoryBlock =
                    VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
                                 MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                GUIMemory.PermanentStorage = Win32State.RenderMemoryBlock;
                GUIMemory.TransientStorage =
                    ((u8 *)GUIMemory.PermanentStorage + GUIMemory.PermanentStorageSize);

                if (GUIMemory.PermanentStorage && GUIMemory.TransientStorage) {
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

                        DWORD MaxControllerCount = 2;
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

                        UpdateAndRender(&Buffer, &GUIMemory, NewInput);

                        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                        HDC DeviceContext = GetDC(Window);
                        Win32DisplayBufferInWindow(DeviceContext, &GlobalBackBuffer,
                                                   Dimension.Width, Dimension.Height);
                        ReleaseDC(Window, DeviceContext);
                    }

                } else {
                    // TODO: Logging
                }

                CloseHandle(Win32ComHandle);
            } else {
                // TODO: Logging
#if DR_INTERNAL
                OutputDebugStringA("Failed to open COM");
#endif
            }
        } else {
            // TODO: Logging
        }
    } else {
        // TODO: Logging
    }
}
