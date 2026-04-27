#if !defined(DRGUI_PLATFORM_H)
#include "types.h"

#if DR_INTERNAL
#define Assert(Expression)                          \
    if (!(Expression)) {                            \
        *(volatile int *)0 = 0;                     \
    }
#else
#define Assert(Expression)
#endif

#define ArrayCount(Arr) (sizeof(Arr) / sizeof((Arr)[0]))

#define Kilobytes(Val) ((Val) * 1024)
#define Megabytes(Val) (Kilobytes(Val) * 1024)
#define Gigabytes(Val) (Megabytes(Val) * 1024)
#define Terabytes(Val) (Gigabytes(Val) * 1024)


typedef struct button_state {
    int HalfTransitionCount;
    b32 EndedDown;
} button_state;

typedef struct controller_input {
    b32 IsConnected;
    b32 IsAnalog;
    r32 LStickAverageX;
    r32 LStickAverageY;

    r32 RStickAverageX;
    r32 RStickAverageY;

    union {
        button_state Buttons[16];
        struct {
            button_state MoveUp;
            button_state MoveDown;
            button_state MoveLeft;
            button_state MoveRight;

            button_state LookUp;
            button_state LookDown;
            button_state LookLeft;
            button_state LookRight;

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

#define BITMAP_BYTES_PER_PIXEL 4
typedef struct offscreen_buffer {
    // NOTE: Pixels are always 32-bits wide
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
} offscreen_buffer;

typedef struct platform_file_handle platform_file_handle;

// TODO: See if this needs more work to be more robust if blocking becomes an issue
typedef struct read_com_result {
    u32 ContentSize;
    void *Contents;
} read_com_result;
#define PLATFORM_READ_COM(name) read_com_result name(void *ComHandle, void* Buffer, u32 PacketSize)
typedef PLATFORM_READ_COM(platform_read_com);

#define PLATFORM_FREE_FILE_MEMORY(name) void name(void *Memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

#define PLATFORM_WRITE_COM(name) b32 name(void *ComHandle, u32 PacketSize, void *Memory)
typedef PLATFORM_WRITE_COM(platform_write_com);

struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue *Queue, void *Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef void platform_add_entry(platform_work_queue *Queue,
                           platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);

struct platform_com_dev {
    void* ComHandle;
    u32 *PortName;
};

typedef struct gui_memory {
    b32 IsInitialized;

    u64 PermanentStorageSize;
    void *PermanentStorage; // Required to be cleared to zero at startup

    u64 TransientStorageSize;
    void *TransientStorage; // Required to be cleared to zero at startup

    platform_work_queue *HighPriorityQueue;
    platform_add_entry *PlatformAddEntry;
    platform_complete_all_work *PlatformCompleteAllWork;
} gui_memory;

#define DRGUI_PLATFORM_H
#endif
