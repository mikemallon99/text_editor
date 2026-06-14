#include <windows.h>
#include "editor.c"
#include "w32_platform.c"

U64 global_perf_count_freq = 0;

////////////////////////
// NOTE: Main

int main(int argc, char **argv)
{
    // Get handles to STDIN and STDOUT.
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE || hStdout == INVALID_HANDLE_VALUE)
    {
        MessageBox(NULL, TEXT("GetStdHandle"), TEXT("Console Error"),
            MB_OK);
        return 1;
    }

    // Turn off the line input and echo input modes
    DWORD fdwOldMode;
    if (!GetConsoleMode(hStdin, &fdwOldMode))
    {
       MessageBox(NULL, TEXT("GetConsoleMode"), TEXT("Console Error"),
           MB_OK);
       return 1;
    }
    DWORD fdwMode = fdwOldMode &
        ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    if (! SetConsoleMode(hStdin, fdwMode))
    {
       MessageBox(NULL, TEXT("SetConsoleMode"), TEXT("Console Error"),
           MB_OK);
       return 1;
    }

    // New screen buffer to draw to
    HANDLE hNewScreenBuffer = CreateConsoleScreenBuffer(
       GENERIC_READ|GENERIC_WRITE,
       FILE_SHARE_READ|FILE_SHARE_WRITE,
       NULL,                    // default security attributes
       CONSOLE_TEXTMODE_BUFFER, // must be TEXTMODE
       NULL);                   // reserved; must be NULL
    if (hNewScreenBuffer == INVALID_HANDLE_VALUE)
    {
        printf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError());
        return 1;
    }

    // Make the new screen buffer the active screen buffer.
    if (!SetConsoleActiveScreenBuffer(hNewScreenBuffer))
    {
        printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
        return 1;
    }

    CONSOLE_SCREEN_BUFFER_INFO screen_buffer_info = {0};
    if (!GetConsoleScreenBufferInfo(hNewScreenBuffer, &screen_buffer_info))
    {
        printf("GetConsoleScreenBufferInfo failed - (%d)\n", GetLastError());
        return 1;
    }

    COORD top_left_coord = {0};
    U32 charinfo_buffer_size = sizeof(CHAR_INFO)*(screen_buffer_info.dwSize.X*screen_buffer_info.dwSize.Y);
    CHAR_INFO *charinfo_buffer = VirtualAlloc(0, charinfo_buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!ReadConsoleOutput(hNewScreenBuffer,        
                           charinfo_buffer,      
                           screen_buffer_info.dwSize,   
                           top_left_coord,  
                           &screen_buffer_info.srWindow))
    {
        printf("ReadConsoleOutput failed - (%d)\n", GetLastError());
        return 1;
    }

    ConsoleBuffer console_buffer = {0};
    console_buffer.width = screen_buffer_info.dwSize.X;
    console_buffer.height = screen_buffer_info.dwSize.Y;
    console_buffer.cursor.x = screen_buffer_info.dwCursorPosition.X;
    console_buffer.cursor.y = screen_buffer_info.dwCursorPosition.Y;
    console_buffer.memory = (U8*)VirtualAlloc(0, console_buffer.width*console_buffer.height, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    AppMemory memory = {0};
    memory.size = MB(256);
    memory.memory = (void*)VirtualAlloc(0, memory.size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    memory.platform_read_file = w32_read_file;
    memory.platform_write_file = w32_write_file;
    Assert(argc == 2);
    memory.cli_input_filepath = (U8*)argv[1];


    // NOTE: Get frequency for the CPU counter
    LARGE_INTEGER perf_count_freq_result;
    QueryPerformanceFrequency(&perf_count_freq_result);
    global_perf_count_freq = perf_count_freq_result.QuadPart;
    F32 update_hz = 60.f;
    F32 target_seconds_per_frame = 1.0f / update_hz;
    LARGE_INTEGER last_counter = w32_get_wall_clock();
    U64 last_cycle_count = __rdtsc();

    // TODO: How to profile this entire loop?
    int is_running = 1;
    // NOTE: Input is like a controller, not events
    Keyboard keyboard = {0};
    while (is_running)
    {
        // Have this here cuz were gonna be looking at the times in a debugger
        LARGE_INTEGER start_counter = w32_get_wall_clock();

        // Check if console buffer changed at all
        GetConsoleScreenBufferInfo(hNewScreenBuffer, &screen_buffer_info);
        if (console_buffer.width != screen_buffer_info.dwSize.X || 
            console_buffer.height != screen_buffer_info.dwSize.Y)
        {
            console_buffer.width = screen_buffer_info.dwSize.X;
            console_buffer.height = screen_buffer_info.dwSize.Y;
            VirtualFree(console_buffer.memory, 0, MEM_RELEASE);
            console_buffer.memory = (U8*)VirtualAlloc(0, console_buffer.width*console_buffer.height, 
                                                      MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            VirtualFree(charinfo_buffer, 0, MEM_RELEASE);
            charinfo_buffer_size = sizeof(CHAR_INFO)*(console_buffer.width*console_buffer.height);
            charinfo_buffer = (CHAR_INFO*)VirtualAlloc(0, charinfo_buffer_size, 
                                                       MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if (!ReadConsoleOutput(hNewScreenBuffer,        
                                   charinfo_buffer,      
                                   screen_buffer_info.dwSize,   
                                   top_left_coord,  
                                   &screen_buffer_info.srWindow))
            {
                printf("ReadConsoleOutput failed - (%d)\n", GetLastError());
                return 1;
            }
        }

        Keyboard keyboard = {0};
        // Check if console window in focus to recieve inputs or not
        HWND console_window = GetConsoleWindow(); 
        HWND foreground_window = GetForegroundWindow(); 
        if (console_window == foreground_window)
        {
            keyboard = w32_get_keyboard_state();
        }

        LARGE_INTEGER setup_counter = w32_get_wall_clock();
        F32 setup_seconds_elapsed = w32_get_seconds_elapsed(start_counter, setup_counter);

        // Update
        app_update(&memory, keyboard, &console_buffer);

        LARGE_INTEGER update_counter = w32_get_wall_clock();
        F32 update_seconds_elapsed = w32_get_seconds_elapsed(setup_counter, update_counter);

        // TODO: Get the app update time here
        // F32 FromBeginToAudioSeconds = w32_get_seconds_elapsed(flip_wall_clock, AudioWallClock);

        // Convert char buffer to CHAR_INFO buffer
        for (U32 i=0; i < console_buffer.height; i++)
        {
            for (U32 j=0; j < console_buffer.width; j++)
            {
                U32 index = console_buffer.width*i + j;
                charinfo_buffer[index].Char.AsciiChar = (CHAR)(console_buffer.memory[index]);
            }
        }

        LARGE_INTEGER console_copy_counter = w32_get_wall_clock();
        F32 console_copy_seconds_elapsed = w32_get_seconds_elapsed(update_counter, console_copy_counter);
        
        // Copy output buffer
        // NOTE: I suspect that this has a latency to it
        if (!WriteConsoleOutput(hNewScreenBuffer,
                                charinfo_buffer,
                                screen_buffer_info.dwSize,
                                top_left_coord,
                                &screen_buffer_info.srWindow))
        {
            printf("WriteConsoleOutput failed - (%d)\n", GetLastError());
            break;
        }

        LARGE_INTEGER write_console_counter = w32_get_wall_clock();
        F32 write_console_seconds_elapsed = w32_get_seconds_elapsed(console_copy_counter, write_console_counter);

        COORD cursor_pos = {console_buffer.cursor.x, console_buffer.cursor.y};
        if (!SetConsoleCursorPosition(hNewScreenBuffer, cursor_pos))
        {
            printf("SetConsoleCursorPosition failed - (%d)\n", GetLastError());
            break;
        }

        LARGE_INTEGER write_cursor_counter = w32_get_wall_clock();
        F32 write_cursor_seconds_elapsed = w32_get_seconds_elapsed(write_console_counter, write_cursor_counter);

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

    // Restore the original console mode.
    SetConsoleMode(hStdin, fdwOldMode);

    // Restore the original active screen buffer.
    if (! SetConsoleActiveScreenBuffer(hStdout))
    {
        printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
        return 1;
    }

    return 0;
}
