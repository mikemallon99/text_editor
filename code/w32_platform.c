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
