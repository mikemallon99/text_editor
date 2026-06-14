#include <windows.h>
#include "editor.c"
#include "render.c"

U64 global_perf_count_freq = 0;

#include "w32_platform.c"

typedef struct
{
    BITMAPINFO info;
    void* memory;
    U32 width;
    U32 height;
    U32 pitch;
    U32 bytes_per_pixel;
} W32OffscreenBuffer; 

// NOTE: this is a global for now
global W32OffscreenBuffer global_backbuffer;
global B32 global_running = 1;

typedef struct
{
    U32 width;
    U32 height;
} W32WindowDimension; 

function W32WindowDimension
w32_get_window_dimension(HWND window)
{
    W32WindowDimension result;
    RECT rect;
    GetClientRect(window, &rect);
    result.width = rect.right - rect.left;
    result.height = rect.bottom - rect.top;
    return result;
}

function void 
w32_resize_dib_section(W32OffscreenBuffer *buffer, int width, int height) 
{
    // TODO: bulletproof this
    // Dont free first, free after, then free first if that fails
    if (buffer->memory) {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    buffer->bytes_per_pixel = 4;
    int memory_size = buffer->bytes_per_pixel * (buffer->width * buffer->height);
    buffer->memory = VirtualAlloc(0, memory_size, MEM_COMMIT, PAGE_READWRITE);

    buffer->pitch = buffer->width*buffer->bytes_per_pixel;
}

function void 
w32_display_buffer_in_window(W32OffscreenBuffer *buffer, HDC device_context, 
                             int window_width, int window_height)
{
    // NOTE: I think this isnt needed?
    // PatBlt(device_context, 0, 0, window_width, 0, BLACKNESS);
    // PatBlt(device_context, 0, OffsetY + OutputHeight, WindowWidth, WindowHeight, BLACKNESS);
    // PatBlt(device_context, 0, 0, OffsetX, WindowHeight, BLACKNESS);
    // PatBlt(device_context, OffsetX + OutputWidth, 0, WindowWidth, WindowHeight, BLACKNESS);

    // NOTE: for prototyping purposes, dont stretch the window
    // so we can see the pixels 1:1 when testing the renderer
    StretchDIBits(
        device_context,
        0, 0, 
        window_width, window_height,
        0, 0, 
        buffer->width, buffer->height,
        buffer->memory,
        &buffer->info,
        DIB_RGB_COLORS, SRCCOPY);
}


LRESULT CALLBACK w32_window_callback(
  HWND window,
  UINT message,
  WPARAM WParam,
  LPARAM LParam)
{
    LRESULT result = 0;

    switch(message)
    {
        case WM_SIZE:
        {
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_DESTROY:
        {
            // TODO: handle this as an error & recreate the window
            global_running = 0;
        } break;

        case WM_CLOSE:
        {
            // TODO: handle this with a message to the user
            global_running = 0;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard came in through non dispatch message");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            int X = paint.rcPaint.left;
            int Y = paint.rcPaint.top;

            W32WindowDimension dimension = w32_get_window_dimension(window);
            w32_display_buffer_in_window(&global_backbuffer, device_context,
                                         dimension.width, dimension.height);
            EndPaint(window, &paint);
        } break;

        default:
        {
            result = DefWindowProc(window, message, WParam, LParam);
        } break;
    }

    return result;
}


int CALLBACK WinMain(
    HINSTANCE instance, 
    HINSTANCE prev_instance, 
    LPSTR CmdLine, 
    int ShowCode) 
{
    w32_resize_dib_section(&global_backbuffer, 1280, 720);

    WNDCLASSA window_class = {0};
    window_class.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.lpfnWndProc = w32_window_callback;
    window_class.hInstance = instance;
    window_class.lpszClassName = "Poppy editor";

    if (!RegisterClassA(&window_class)) 
    {
        printf("RegisterClassA failed - (%d)\n", GetLastError());
    }

    U32 window_width = global_backbuffer.width; 
    U32 window_height = global_backbuffer.height; 
    HWND window = CreateWindowEx(
        0,
        window_class.lpszClassName,
        "poppy editor",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        window_width,
        window_height,
        0,
        0,
        instance,
        0
    );

    if (!window) 
    {
        printf("CreateWindowEx failed - (%d)\n", GetLastError());
    }

    // NOTE: Commented out cuz im just forcing 60hz
    // We should only need one DC because we specified OWN_DC
    // HDC device_context = GetDC(window);
    // int monitor_refresh_hz = 60;
    // int w32_refresh_rate = GetDeviceCaps(device_context, VREFRESH);
    // if (w32_refresh_rate > 1)
    // {
    //     monitor_refresh_hz = w32_refresh_rate;
    // }
    // F32 update_hz = (monitor_refresh_hz / 1.0f);
    // F32 target_seconds_per_frame = 1.0f / update_hz;

    // TODO: Make this not global?
    LARGE_INTEGER perf_count_freq_result;
    QueryPerformanceFrequency(&perf_count_freq_result);
    global_perf_count_freq = perf_count_freq_result.QuadPart;

    ConsoleBuffer console_buffer = {0};
    // TODO: How to make console buffer fill the window size?
    console_buffer.width = 120;
    console_buffer.height = 80;
    console_buffer.memory = (U8*)VirtualAlloc(0, console_buffer.width*console_buffer.height, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    AppMemory memory = {0};
    memory.size = MB(256);
    memory.memory = (void*)VirtualAlloc(0, memory.size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    memory.platform_read_file = w32_read_file;
    memory.platform_write_file = w32_write_file;
    memory.cli_input_filepath = (U8*)"W:/text_editor/tests/win32_platform.c";

    // NOTE: Get frequency for the CPU counter
    F32 update_hz = 60.f;
    F32 target_seconds_per_frame = 1.0f / update_hz;
    LARGE_INTEGER last_counter = w32_get_wall_clock();
    U64 last_cycle_count = __rdtsc();

    // TODO: How to profile this entire loop?
    // NOTE: Input is like a controller, not events
    Keyboard keyboard = {0};
    global_running = 1;
    while (global_running)
    {
        // Have this here cuz were gonna be looking at the times in a debugger
        LARGE_INTEGER start_counter = w32_get_wall_clock();

        // TODO: Since were using wingui now we can use events to read the keyboard
        Keyboard keyboard = {0};
        // Check if console window in focus to recieve inputs or not
        HWND foreground_window = GetForegroundWindow(); 
        if (window == foreground_window)
        {
            keyboard = w32_get_keyboard_state();
        }

        // Process pending messages to prevent the not responding:
        MSG message;
        while(PeekMessage(
                &message,
                0, // 0 retrieves messages for any window that belongs to thread
                0, // Returns all available messages
                0, // Returns all available messages
                PM_REMOVE // Removes messages from the queue when processing them
                )) 
        {
            if (message.message == WM_QUIT) 
            {
                global_running = 0;
            }
            else
            {
                switch(message.message)
                {
                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                    {
                    } break;
                    default:
                    {
                        // Gets message ready to send out
                        TranslateMessage(&message);
                        // Actually sends it out
                        DispatchMessage(&message);
                    } break;
                }
            }
        }

        LARGE_INTEGER setup_counter = w32_get_wall_clock();
        F32 setup_seconds_elapsed = w32_get_seconds_elapsed(start_counter, setup_counter);

        // Update
        app_update(&memory, keyboard, &console_buffer);

        LARGE_INTEGER update_counter = w32_get_wall_clock();
        F32 update_seconds_elapsed = w32_get_seconds_elapsed(setup_counter, update_counter);

        // Render
        VideoMemory video_memory = {0};
        video_memory.memory = global_backbuffer.memory;
        video_memory.width = global_backbuffer.width;
        video_memory.height = global_backbuffer.height;
        video_memory.pitch = global_backbuffer.pitch;
        video_memory.bytes_per_pixel = global_backbuffer.bytes_per_pixel;
        memory_zero(video_memory.memory, video_memory.width*video_memory.height*video_memory.bytes_per_pixel);
        AppState *app_state = (AppState*)memory.memory;
        app_render(console_buffer, video_memory, app_state->font);

        HDC device_context = GetDC(window);
        W32WindowDimension dimension = w32_get_window_dimension(window);
        // TODO: Fix the weird image stretching
        // TODO: Fix window not responding
        w32_display_buffer_in_window(&global_backbuffer, device_context, 
                                     dimension.width, dimension.height);

        // Grab the new counters
        LARGE_INTEGER end_counter = w32_get_wall_clock();
        U64 end_cycle_count = __rdtsc();

        F32 end_seconds_elapsed = w32_get_seconds_elapsed(update_counter, end_counter);
        F32 frame_seconds_elapsed = w32_get_seconds_elapsed(start_counter, end_counter);
        F64 ms_per_frame = 1000.0f*frame_seconds_elapsed;
        F64 fps = 1.0f/frame_seconds_elapsed;
        U64 cycles_elapsed = end_cycle_count - last_cycle_count;
        F64 mcpf = ((F64)(cycles_elapsed) / (1000.0f * 1000.0f));
        printf("%.02fms/f, %.02ff/s, %.02fmc/f\n", ms_per_frame, fps, mcpf);

        // Flip the counters
        last_counter = end_counter;
        last_cycle_count = end_cycle_count;
    }

    return 0;
}