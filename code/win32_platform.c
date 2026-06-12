#include <windows.h>
#include "editor.c"

U64 global_perf_count_freq = 0;

function U32
w32_virtual_to_keycode(U32 virtual_code)
{
    U32 result = 1000;
    if (virtual_code >= '0' && virtual_code <= '9')
    {
        result = virtual_code - '0';
    }
    else if (virtual_code >= 'A' && virtual_code <= 'Z')
    {
        result = 10 + virtual_code - 'A';
    }
    else if (virtual_code == VK_OEM_1)
    {
        result = KEY_SEMICOLON;
    }
    else if (virtual_code == VK_OEM_PLUS)
    {
        result = KEY_PLUS;
    }
    else if (virtual_code == VK_OEM_COMMA)
    {
        result = KEY_COMMA;
    }
    else if (virtual_code == VK_OEM_MINUS)
    {
        result = KEY_MINUS;
    }
    else if (virtual_code == VK_OEM_PERIOD)
    {
        result = KEY_PERIOD;
    }
    else if (virtual_code == VK_OEM_2)
    {
        result = KEY_SLASH;
    }
    else if (virtual_code == VK_OEM_3)
    {
        result = KEY_BACKTICK;
    }
    else if (virtual_code == VK_OEM_4)
    {
        result = KEY_LBRACE;
    }
    else if (virtual_code == VK_OEM_5)
    {
        result = KEY_BACKSLASH;
    }
    else if (virtual_code == VK_OEM_6)
    {
        result = KEY_RBRACE;
    }
    else if (virtual_code == VK_OEM_7)
    {
        result = KEY_QUOTE;
    }
    else if (virtual_code == VK_SPACE)
    {
        result = KEY_SPACE;
    }
    else if (virtual_code == VK_RETURN)
    {
        result = KEY_RETURN;
    }
    else if (virtual_code == VK_CONTROL)
    {
        result = KEY_CTRL;
    }
    else if (virtual_code == VK_MENU)
    {
        result = KEY_ALT;
    }
    else if (virtual_code == VK_ESCAPE)
    {
        result = KEY_ESC;
    }
    else if (virtual_code == VK_BACK)
    {
        result = KEY_BACKSPACE;
    }
    else if (virtual_code == VK_SHIFT)
    {
        result = KEY_SHIFT;
    }
    else if (virtual_code == VK_TAB)
    {
        result = KEY_TAB;
    }
    return result;
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

function Keyboard
w32_get_keyboard_state()
{
    Keyboard result = {0};
    // 0-9
    for (U32 vk='0'; vk <= '9'; vk++)
    {
        if (GetAsyncKeyState(vk) & 0x8000) 
        {
            U32 keycode = vk - '0';
            result.keys[keycode].is_down = 1;
        }
    }
    // A-Z
    for (U32 vk='A'; vk <= 'Z'; vk++)
    {
        if (GetAsyncKeyState(vk) & 0x8000) 
        {
            U32 keycode = vk - 'A' + 10;
            result.keys[keycode].is_down = 1;
        }
    }
    // Special chars
    if (GetAsyncKeyState(VK_OEM_1) & 0x8000) 
    {
        result.keys[KEY_SEMICOLON].is_down = 1;
    }
    if (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) 
    {
        result.keys[KEY_PLUS].is_down = 1;
    }
    if (GetAsyncKeyState(VK_OEM_COMMA) & 0x8000) 
    {
        result.keys[KEY_COMMA].is_down = 1;
    }
    if (GetAsyncKeyState(VK_OEM_MINUS) & 0x8000) 
    {
        result.keys[KEY_MINUS].is_down = 1;
    }
    if (GetAsyncKeyState(VK_OEM_PERIOD) & 0x8000) 
    {
        result.keys[KEY_PERIOD].is_down = 1;
    }
    if (GetAsyncKeyState(VK_OEM_2) & 0x8000) 
    {
        result.keys[KEY_SLASH].is_down = 1;
    }
    if (GetAsyncKeyState(VK_OEM_3) & 0x8000) 
    {
        result.keys[KEY_BACKTICK].is_down = 1;
    }
    if (GetAsyncKeyState(VK_OEM_4) & 0x8000) 
    {
        result.keys[KEY_LBRACE].is_down = 1;
    }
    if (GetAsyncKeyState(VK_OEM_5) & 0x8000) 
    {
        result.keys[KEY_BACKSLASH].is_down = 1;
    }
    if (GetAsyncKeyState(VK_OEM_6) & 0x8000) 
    {
        result.keys[KEY_RBRACE].is_down = 1;
    }
    if (GetAsyncKeyState(VK_OEM_7) & 0x8000) 
    {
        result.keys[KEY_QUOTE].is_down = 1;
    }
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) 
    {
        result.keys[KEY_SPACE].is_down = 1;
    }
    if (GetAsyncKeyState(VK_RETURN) & 0x8000) 
    {
        result.keys[KEY_RETURN].is_down = 1;
    }
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000) 
    {
        result.keys[KEY_CTRL].is_down = 1;
    }
    if (GetAsyncKeyState(VK_MENU) & 0x8000) 
    {
        result.keys[KEY_ALT].is_down = 1;
    }
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) 
    {
        result.keys[KEY_ESC].is_down = 1;
    }
    if (GetAsyncKeyState(VK_BACK) & 0x8000) 
    {
        result.keys[KEY_BACKSPACE].is_down = 1;
    }
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000) 
    {
        result.keys[KEY_SHIFT].is_down = 1;
    }
    if (GetAsyncKeyState(VK_TAB) & 0x8000) 
    {
        result.keys[KEY_TAB].is_down = 1;
    }
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

PLATFORM_WRITE_FILE(w32_write_file)
{
    HANDLE handle = CreateFileA(file_path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle != INVALID_HANDLE_VALUE)
    {
        DWORD bytes_written = 0;
        BOOL write_err = WriteFile(handle, buffer, buffer_size, &bytes_written, 0);
        if (!write_err)
        {
            printf("WriteFile failed - (%d)\n", GetLastError());
        }

        if (bytes_written != buffer_size)
        {
            printf("w32_write_file error - bytes_written != buffer_size\n");
        }
        CloseHandle(handle);
    }
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

        // NOTE: This input gathering method has bugs on combo keys (shift+4 missing key up)
        // // NOTE: Do it like this so we dont have to block waiting for input
        // INPUT_RECORD input_record[16] = {0};
        // DWORD cRead;
        // // Reset key input
        // // keyboard.key_type = 0;
        // // keyboard.key_value = 0;
        // PeekConsoleInput(hStdin, (INPUT_RECORD*)&input_record, 16, &cRead);
        // if (cRead > 0)
        // {
        //     ReadConsoleInput(hStdin, (INPUT_RECORD*)&input_record, 16, &cRead);
        //     for (U32 i=0; i < cRead; i++)
        //     {
        //         // TODO: For some reason were not getting notified that '4' was released
        //         if (input_record[i].EventType == KEY_EVENT)
        //         {
        //             B32 is_down = input_record[i].Event.KeyEvent.bKeyDown;
        //             DWORD vk_code = input_record[i].Event.KeyEvent.wVirtualKeyCode;
        //             U32 key_index = w32_virtual_to_keycode(vk_code);
        //             if (key_index < KEY_COUNT)
        //             {
        //                 keyboard.keys[key_index].half_trans_count += keyboard.keys[key_index].is_down ^ is_down;
        //                 keyboard.keys[key_index].is_down = is_down;
        //             }
        //         }
        //     }
        // }

        Keyboard keyboard = {0};
        // Check if console window in focus to recieve inputs or not
        HWND console_window = GetConsoleWindow(); 
        HWND foreground_window = GetForegroundWindow(); 
        if (console_window == foreground_window)
        {
            keyboard = w32_get_keyboard_state();
        }

        // Update
        app_update(&memory, keyboard, &console_buffer);

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
