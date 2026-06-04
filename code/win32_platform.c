#include <windows.h>
#include "editor.c"

U64 global_perf_count_freq = 0;

function U8
w32_virtual_to_ascii(U32 virtual_code)
{
    U8 result = 0;
    // a = 0x41
}

function B32
is_special(U8 c)
{
    B32 result = 0;
    String special_chars = str_lit("!@#$%^&*()_+-=<>,.?`~;:'\"\\|[]{}");
    for (U32 i=0; i < special_chars.size; i++)
    {
        if (c == special_chars.data[i])
        {
            result = 1;
            break;
        }
    }
    return result;
}

function F32
w32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    F32 Result = (((F32)(end.QuadPart - start.QuadPart)) / 
                                        (F32)global_perf_count_freq);

    return Result;
}

function LARGE_INTEGER
w32_get_wall_clock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

function U32
safe_truncate_u64(U64 value)
{
    // TODO: defines for max values
    Assert(value <= 0xFFFFFFFF);
    U32 result = (U32)value;
    return result;
}

// NOTE: Returns 0 if buffer isnt big enough or other failure
//       Caller will then pass a properly sized buffer based on bytes_read_out
PLATFORM_READ_FILE(w32_read_file)
{
    U32 result = 0;
    HANDLE handle = CreateFileA(file_path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (handle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER file_size;
        if (GetFileSizeEx(handle, &file_size))
        {
            *file_size_out = safe_truncate_u64(file_size.QuadPart);
            if (*file_size_out <= buffer_size)
            {
                DWORD BytesRead = 0;
                if (ReadFile(handle, buffer, *file_size_out, &BytesRead, 0) && 
                    (*file_size_out == BytesRead))
                {
                    result = 1;
                }
            }
        }
        else
        {
            // File does not exist
            Assert(0);
        }

        CloseHandle(handle);
    }
    return result;
}

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
    U32 char_buffer_size = sizeof(CHAR_INFO)*(screen_buffer_info.dwSize.X*screen_buffer_info.dwSize.Y);
    CHAR_INFO *chiBuffer = VirtualAlloc(0, char_buffer_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!ReadConsoleOutput(hNewScreenBuffer,        
                           chiBuffer,      
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
    memory.size = MB(64);
    memory.memory = (void*)VirtualAlloc(0, memory.size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    memory.platform_read_file = w32_read_file;
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
    Input input = {0};
    while (is_running)
    {
        // NOTE: Do it like this so we dont have to block waiting for input
        INPUT_RECORD input_record[16] = {0};
        DWORD cRead;
        // Reset key input
        input.key_type = 0;
        input.key_value = 0;
        PeekConsoleInput(hStdin, (INPUT_RECORD*)&input_record, 16, &cRead);
        if (cRead > 0)
        {
            ReadConsoleInput(hStdin, (INPUT_RECORD*)&input_record, 16, &cRead);
            for (U32 i=0; i < cRead; i++)
            {
                // TODO: This shitz broken, Ctrl+A doesnt work
                if (input_record[i].EventType == KEY_EVENT &&
                    input_record[i].Event.KeyEvent.wVirtualKeyCode == VK_CONTROL)
                {
                    input.ctrl_down = input_record[i].Event.KeyEvent.bKeyDown;
                }
                else if (input_record[i].EventType == KEY_EVENT &&
                         input_record[i].Event.KeyEvent.bKeyDown == TRUE)
                {
                    switch (input_record[i].Event.KeyEvent.wVirtualKeyCode)
                    {
                        case VK_BACK:
                        {
                            input.key_type = KEY_BACKSPACE;
                        } break;
                        case VK_TAB:
                        {
                            input.key_type = KEY_TAB;
                        } break;
                        case VK_RETURN:
                        {
                            input.key_type = KEY_RETURN;
                        } break;
                        case VK_LEFT:
                        {
                            input.key_type = KEY_LEFTARROW;
                        } break;
                        case VK_RIGHT:
                        {
                            input.key_type = KEY_RIGHTARROW;
                        } break;
                        case VK_UP:
                        {
                            input.key_type = KEY_UPARROW;
                        } break;
                        case VK_DOWN:
                        {
                            input.key_type = KEY_DOWNARROW;
                        } break;
                        case VK_ESCAPE:
                        {
                            input.key_type = KEY_ESC;
                        } break;
                        // TODO: Alt key
                        default:
                        {
                            U8 ascii_value = input_record[i].Event.KeyEvent.uChar.AsciiChar;
                            if (is_alpha(ascii_value) || is_digit(ascii_value) || is_special(ascii_value) || is_whitespace(ascii_value))
                            {
                                input.key_type = KEY_CHAR;
                                input.key_value = ascii_value;
                            }
                        } break;
                    }
                }
            }
        }

        // Update
        app_update(&memory, input, &console_buffer);

        // TODO: Get the app update time here
        // F32 FromBeginToAudioSeconds = w32_get_seconds_elapsed(flip_wall_clock, AudioWallClock);

        // Convert char buffer to CHAR_INFO buffer
        for (U32 i=0; i < console_buffer.height; i++)
        {
            for (U32 j=0; j < console_buffer.width; j++)
            {
                U32 index = console_buffer.width*i + j;
                chiBuffer[index].Char.AsciiChar = (CHAR)(console_buffer.memory[index]);
            }
        }
        
        // Copy output buffer
        // NOTE: I suspect that this has a latency to it
        if (!WriteConsoleOutput(hNewScreenBuffer,
                                chiBuffer,
                                screen_buffer_info.dwSize,
                                top_left_coord,
                                &screen_buffer_info.srWindow))
        {
            printf("WriteConsoleOutput failed - (%d)\n", GetLastError());
            break;
        }

        COORD cursor_pos = {console_buffer.cursor.x, console_buffer.cursor.y};
        if (!SetConsoleCursorPosition(hNewScreenBuffer, cursor_pos))
        {
            printf("SetConsoleCursorPosition failed - (%d)\n", GetLastError());
            break;
        }

        // Grab the new counters
        LARGE_INTEGER end_counter = w32_get_wall_clock();
        U64 end_cycle_count = __rdtsc();

        F32 work_seconds_elapsed = w32_get_seconds_elapsed(last_counter, end_counter);
        F32 seconds_elapsed_for_frame = work_seconds_elapsed;
        F64 ms_per_frame = 1000.0f*seconds_elapsed_for_frame;
        F64 fps = 1.0f/seconds_elapsed_for_frame;
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
