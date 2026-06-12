#include "types.c"
#include "string.c"
#include "lexer.c"

#include "editor.h"

// Things u can get from memory pointer:
//  - String index
//  - String cursor (x,y)
//  - Token index (Token, before, after)

function String
read_file_to_string(PlatformReadFileFunc *read_file_func, Arena *arena, String filepath)
{
    String result = {0};
    U32 bytes_read;
    U8 *buffer = (U8*)arena->base + arena->pos;
    U32 buffer_size = arena->cap - arena->pos;
    // NOTE: This assumes 'filepath' String still has underlying cstring data
    U32 read_file_result = read_file_func(filepath.data, 
                                          buffer, buffer_size, 
                                          &bytes_read);
    // TODO: Need to handle the case where input buffer isnt big enough
    Assert(read_file_result);
    arena->pos += bytes_read;
    result.data = buffer;
    result.size = bytes_read;
    return result;
}

function Vec2i
get_string_cursor(StringArray string_lines, Vec2i base_cursor)
{
    Vec2i result = {0};
    result.y = base_cursor.y;
    if (result.y >= string_lines.count)
    {
        result.y = string_lines.count-1;
    }
    result.x = base_cursor.x;
    String string_line = string_lines.strings[result.y];
    if (result.x > string_line.size)
    {
        result.x = string_line.size;
    }
    return result;
}

typedef struct
{
    Token *token;
    Token *next;
    Token *prev;
} TokenIndex;


// Virtual cursor
//  - String Cursor -> String index -> String cursor
//  - String index -> token index -> String index

function U8*
str_cursor_to_addr(StringArray lines, Vec2i cursor)
{
    U8 *result = 0;
    Assert(cursor.y < lines.count);
    Assert(cursor.x <= lines.strings[cursor.y].size);
    String line = lines.strings[cursor.y];
    result = &line.data[cursor.x];
    return result;
}

function Vec2i
str_addr_to_cursor(StringArray lines, U8* addr)
{
    Vec2i result = {0};
    for (U32 i=0; i < lines.count; i++)
    {
        String line = lines.strings[i];
        if (addr >= line.data &&
            addr < (line.data+line.size))
        {
            result.x = addr - line.data;
            result.y = i;
            break;
        }
    }
    return result;
}

function U8
keycode_to_char(U32 keycode, B32 shift_down)
{
    U8 result;
    if (keycode >= 0 && keycode <= 9 && !shift_down)
    {
        result = '0' + keycode;
    }
    else if (keycode >= 0 && keycode <= 9 && shift_down)
    {
        switch (keycode)
        {
            case 1:
                result = '!';
                break;
            case 2:
                result = '@';
                break;
            case 3:
                result = '#';
                break;
            case 4:
                result = '$';
                break;
            case 5:
                result = '%';
                break;
            case 6:
                result = '^';
                break;
            case 7:
                result = '&';
                break;
            case 8:
                result = '*';
                break;
            case 9:
                result = '(';
                break;
            case 0:
                result = ')';
                break;
            default:
                Assert(0);
        }
    }
    else if (keycode >= 10 && keycode <= 35)
    {
        if (shift_down)
        {
            result = 'A' + (keycode - 10);
        }
        else
        {
            result = 'a' + (keycode - 10);
        }
    }
    else
    {
        switch (keycode)
        {
            case KEY_SEMICOLON:
            {
                if (shift_down)
                {
                    result = ':';
                }
                else
                {
                    result = ';';
                }
            } break;
            case KEY_PLUS:
            {
                if (shift_down)
                {
                    result = '+';
                }
                else
                {
                    result = '=';
                }
            } break;
            case KEY_COMMA:
            {
                if (shift_down)
                {
                    result = '<';
                }
                else
                {
                    result = ',';
                }
            } break;
            case KEY_MINUS:
            {
                if (shift_down)
                {
                    result = '_';
                }
                else
                {
                    result = '-';
                }
            } break;
            case KEY_PERIOD:
            {
                if (shift_down)
                {
                    result = '>';
                }
                else
                {
                    result = '.';
                }
            } break;
            case KEY_SLASH:
            {
                if (shift_down)
                {
                    result = '?';
                }
                else
                {
                    result = '/';
                }
            } break;
            case KEY_BACKTICK:
            {
                if (shift_down)
                {
                    result = '~';
                }
                else
                {
                    result = '`';
                }
            } break;
            case KEY_LBRACE:
            {
                if (shift_down)
                {
                    result = '{';
                }
                else
                {
                    result = '[';
                }
            } break;
            case KEY_BACKSLASH:
            {
                if (shift_down)
                {
                    result = '|';
                }
                else
                {
                    result = '\\';
                }
            } break;
            case KEY_RBRACE:
            {
                if (shift_down)
                {
                    result = '}';
                }
                else
                {
                    result = ']';
                }
            } break;
            case KEY_QUOTE:
            {
                if (shift_down)
                {
                    result = '"';
                }
                else
                {
                    result = '\'';
                }
            } break;
            case KEY_SPACE:
            {
                result = ' ';
            } break;
            default:
                Assert(0);
        }
    }
    return result;
}

function void
app_update(AppMemory *memory, Keyboard keyboard, ConsoleBuffer *console_buffer)
{
    AppState *app_state = (AppState*)memory->memory;
    if (!app_state->is_initialized)
    {
        void *scratch_arena_base = (void*)((U8*)memory->memory + sizeof(AppState));
        U32 scratch_arena_size = MB(16);
        app_state->scratch_arena = arena_make(scratch_arena_base, scratch_arena_size);

        void *string_arena_base = (void*)((U8*)scratch_arena_base + scratch_arena_size);
        U32 string_arena_size = MB(16);
        app_state->string_arena = arena_make(string_arena_base, string_arena_size);

        // Read file
        app_state->filepath = (String){memory->cli_input_filepath, strlen(memory->cli_input_filepath)};
        String file_contents = read_file_to_string(memory->platform_read_file, &app_state->scratch_arena, app_state->filepath);

        // Convert file to internal string representation (CRLF -> LF)
        app_state->string = string_replace(&app_state->string_arena, file_contents, str_lit("\r\n"), str_lit("\n"));

        app_state->last_keyboard = keyboard;

        app_state->is_initialized = 1;
    }

    // TODO:
    // - Need visual way to debug input gathering
    // - undo history
    // - file system explorer
    // - multiple buffers
    // - e/y scrolling
    // - change word
    // - change inside thing
    // - "file saved" notification
    // - hold down key

    // NOTE: Cursor XY needed for memory on hugging line end
    // Cursor XY -> String XY
    StringArray string_lines = string_split(&app_state->scratch_arena, app_state->string, '\n');
    Vec2i string_cursor = get_string_cursor(string_lines, app_state->base_cursor);

    // NOTE: Tokenize everything for vim motions
    TokenArray token_array = tokenize_string(&app_state->scratch_arena, app_state->string);
    U8 *string_data_ptr = str_cursor_to_addr(string_lines, string_cursor);
    TokenIndex token_index = {0};
    for (U32 i=0; i < token_array.count; i++)
    {
        Token *token = &token_array.tokens[i];
        // idx before token?
        if (string_data_ptr < token->value.data)
        {
            token_index.next = token;
            if (i > 0)
            {
                token_index.prev = &token_array.tokens[i-1];
            }
            break;
        }

        // idx in token?
        if ((string_data_ptr >= token->value.data) &&
            (string_data_ptr < (token->value.data + token->value.size)))
        {
            token_index.token = token;
            if (i > 0)
            {
                token_index.prev = &token_array.tokens[i-1];
            }
            if (i+1 < token_array.count)
            {
                token_index.next = &token_array.tokens[i+1];
            }
            break;
        }
    }
    // If nothing found in loop
    if (!token_index.token && !token_index.next && !token_index.prev)
    {
        // idx after last token?
        Token *last_token = &token_array.tokens[token_array.count-1];
        if (string_data_ptr >= (last_token->value.data+last_token->value.size))
        {
            token_index.prev = last_token;
        }
    }

    // TODO: Hold down a key for multiple inputs to register
    for (U32 keycode=0; keycode < KEY_COUNT; keycode++)
    {
        if (keyboard.keys[keycode].is_down)
        {
            app_state->frames_key_down[keycode] += 1;
        }
        else
        {
            app_state->frames_key_down[keycode] = 0;
        }

        // TODO: Lock framerate to 60
        if ((keyboard.keys[keycode].is_down && !app_state->last_keyboard.keys[keycode].is_down) ||
            (app_state->frames_key_down[keycode] > 10 && app_state->frames_key_down[keycode] % 3 == 0))
        {
            if (app_state->input_mode == INPUTMODE_VISUAL && keycode < KEY_RETURN)
            {
                app_state->input_stack[app_state->input_stack_top++] = keycode_to_char(keycode, keyboard.keys[KEY_SHIFT].is_down);
            }
            else if (app_state->input_mode == INPUTMODE_INSERT)
            {
                if (keycode < KEY_RETURN)
                {
                    U8 *string_addr = str_cursor_to_addr(string_lines, string_cursor);
                    U32 string_index = string_addr - app_state->string.data;
                    // NOTE: Cursor puts at current position and then increments
                    string_insert_char(&app_state->string_arena, 
                                       &app_state->string, 
                                       string_index, 
                                       keycode_to_char(keycode, keyboard.keys[KEY_SHIFT].is_down));
                    app_state->base_cursor.x += 1;
                }
                else if (keycode == KEY_RETURN)
                {
                    U8 *string_addr = str_cursor_to_addr(string_lines, string_cursor);
                    U32 string_index = string_addr - app_state->string.data;
                    string_insert_char(&app_state->string_arena, 
                                    &app_state->string, 
                                    string_index, 
                                    '\n');
                    app_state->base_cursor.x = 0;
                    app_state->base_cursor.y += 1;
                }
                else if (keycode == KEY_BACKSPACE)
                {
                    Vec2i backspace_cursor = {0};
                    // Wrap to previous line if at x=0
                    if (string_cursor.x == 0)
                    {
                        backspace_cursor.y = string_cursor.y - 1;
                        backspace_cursor.x = string_lines.strings[backspace_cursor.y].size+1;
                    }
                    else
                    {
                        backspace_cursor.x = string_cursor.x-1;
                        backspace_cursor.y = string_cursor.y;
                    }
                    U8 *string_addr = str_cursor_to_addr(string_lines, backspace_cursor);
                    U32 string_index = string_addr - app_state->string.data;
                    string_remove_index(&app_state->string_arena, 
                                        &app_state->string, 
                                        string_index);
                    app_state->base_cursor = backspace_cursor;
                }
                else if (keycode == KEY_ESC)
                {
                    app_state->input_mode = INPUTMODE_VISUAL;
                }
            }
        }
    }

    if ((app_state->input_mode == INPUTMODE_VISUAL) && app_state->input_stack_top)
    {
        U8 top_input = app_state->input_stack[app_state->input_stack_top-1];
        // Left
        if (top_input == 'h')
        {
            // NOTE: Dont go up to previous line if you hit left
            if (string_cursor.x > 0)
            {
                app_state->base_cursor.x = string_cursor.x - 1;
            }
            app_state->input_stack_top = 0;
        }
        // Right
        else if (top_input == 'l')
        {
            // NOTE: Notice that were calculating off the string XY, not the cursor XY
            String string_line = string_lines.strings[string_cursor.y];
            // NOTE: string_line.size-1, because visual mode can only reach the last char
            if (string_cursor.x < string_line.size-1)
            {
                app_state->base_cursor.x = string_cursor.x + 1;
            }
            app_state->input_stack_top = 0;
        }
        // Up
        else if (top_input == 'k')
        {
            if (string_cursor.y > 0)
            {
                app_state->base_cursor.y = string_cursor.y - 1;
            }
            app_state->input_stack_top = 0;
        }
        // Down
        else if (top_input == 'j')
        {
            // Go to the next line at the x value below
            if (string_cursor.y < string_lines.count-1)
            {
                app_state->base_cursor.y = string_cursor.y + 1;
            }
            app_state->input_stack_top = 0;
        }
        // Insert
        else if (top_input == 'i')
        {
            app_state->input_mode = INPUTMODE_INSERT;
            app_state->input_stack_top = 0;
        }
        ////////////////////////////
        // MOTIONS
        // Start/end of line
        else if (top_input == '0')
        {
            app_state->base_cursor.x = 0;
            app_state->input_stack_top = 0;
        }
        else if (top_input == '$')
        {
            // TODO: Sticks to EOL unintentionally (even tho this matches Vim)
            String string_line = string_lines.strings[string_cursor.y];
            app_state->base_cursor.x = string_line.size-1;
            app_state->input_stack_top = 0;
        }
        // Start/end of file
        else if (app_state->input_stack_top == 2 &&
                 app_state->input_stack[0] == 'g' &&
                 app_state->input_stack[1] == 'g')
        {
            app_state->base_cursor.x = 0;
            app_state->base_cursor.y = 0;
            app_state->input_stack_top = 0;
        }
        else if (top_input == 'G')
        {
            app_state->base_cursor.x = 0;
            app_state->base_cursor.y = string_lines.count-1;
            app_state->input_stack_top = 0;
        }
        // Next/prev word
        else if (top_input == 'w')
        {
            // Go to start of next token
            if (token_index.next)
            {
                U8 *token_start_ptr = token_index.next->value.data;
                app_state->base_cursor = str_addr_to_cursor(string_lines, token_start_ptr);
            }
            app_state->input_stack_top = 0;
        }
        else if (top_input == 'b')
        {
            // Go to start of current or previous token
            U8 *string_addr = str_cursor_to_addr(string_lines, string_cursor);
            U8 *token_start_ptr = 0;
            // Check if at the start of the current token
            if (token_index.prev)
            {
                if (token_index.token && (string_addr == token_index.token->value.data))
                {
                    token_start_ptr = token_index.prev->value.data;
                }
                else
                {
                    token_start_ptr = token_index.prev->value.data;
                }
            }
            app_state->base_cursor = str_addr_to_cursor(string_lines, token_start_ptr);
            app_state->input_stack_top = 0;
        }
        // Delete line
        else if (app_state->input_stack_top == 2 &&
                 app_state->input_stack[0] == 'd' &&
                 app_state->input_stack[1] == 'd')
        {
            // BUG: This crashes on the last line
            // TODO: I feel like we could be using memory addresses instead of indices, idk tho
            U8 *range_start_addr = string_lines.strings[string_cursor.y].data;
            U8 *range_end_addr = 0;
            if (string_cursor.y+1 < string_lines.count)
            {
                range_end_addr = string_lines.strings[string_cursor.y+1].data;
            }
            else
            {
                range_end_addr = string_lines.strings[string_cursor.y].data + string_lines.strings[string_cursor.y].size;
            }
            U32 range_start = range_start_addr - app_state->string.data;
            U32 range_end = range_end_addr - app_state->string.data;
            string_delete_range(&app_state->string_arena, 
                                &app_state->string, 
                                range_start, 
                                range_end);
            app_state->input_stack_top = 0;
        }
        // Delete rest of line
        else if (top_input == 'D')
        {
            U8 *range_start_addr = str_cursor_to_addr(string_lines, string_cursor);
            // NOTE: Dont delete the new line
            U8 *range_end_addr = string_lines.strings[string_cursor.y].data + string_lines.strings[string_cursor.y].size;
            U32 range_start = range_start_addr - app_state->string.data;
            U32 range_end = range_end_addr - app_state->string.data;
            string_delete_range(&app_state->string_arena, 
                                &app_state->string, 
                                range_start, 
                                range_end);
            app_state->input_stack_top = 0;
        }
        // Change rest of line
        else if (top_input == 'C')
        {
            U8 *range_start_addr = str_cursor_to_addr(string_lines, string_cursor);
            // NOTE: Dont delete the new line
            U8 *range_end_addr = string_lines.strings[string_cursor.y].data + string_lines.strings[string_cursor.y].size;
            U32 range_start = range_start_addr - app_state->string.data;
            U32 range_end = range_end_addr - app_state->string.data;
            string_delete_range(&app_state->string_arena, 
                                &app_state->string, 
                                range_start, 
                                range_end);
            app_state->input_mode = INPUTMODE_INSERT;
            app_state->input_stack_top = 0;
        }
        // Up/down half a page
        else if (keyboard.keys[KEY_CTRL].is_down && top_input == 'd')
        {
            U32 half_page = console_buffer->height/2;
            app_state->view_y += half_page;
            if (app_state->view_y+console_buffer->height >= string_lines.count)
            {
                app_state->view_y = string_lines.count - console_buffer->height;
            }
            app_state->base_cursor.y += half_page;
            if (app_state->base_cursor.y >= string_lines.count)
            {
                app_state->base_cursor.y = string_lines.count - 1;
            }
            app_state->input_stack_top = 0;
        }
        else if (keyboard.keys[KEY_CTRL].is_down && top_input == 'u')
        {
            U32 half_page = console_buffer->height/2;
            if (app_state->view_y < half_page)
            {
                app_state->view_y = 0;
            }
            else
            {
                app_state->view_y -= half_page;
            }

            if (app_state->base_cursor.y < half_page)
            {
                app_state->base_cursor.y = 0;
            }
            else
            {
                app_state->base_cursor.y -= half_page;
            }
            app_state->input_stack_top = 0;
        }
        // Save file
        else if (keyboard.keys[KEY_CTRL].is_down && top_input == 's')
        {
            memory->platform_write_file(app_state->filepath.data, app_state->string.data, app_state->string.size);
        }
    }

    // Update view based on latest cursor
    U32 view_bottom = app_state->view_y + console_buffer->height;
    if (app_state->base_cursor.y < app_state->view_y)
    {
        app_state->view_y = app_state->base_cursor.y;
    }
    else if (app_state->base_cursor.y >= view_bottom)
    {
        app_state->view_y = (app_state->base_cursor.y + 1) - console_buffer->height;
    }

    // Render

    // NOTE: Internally store a string interface, render as XY buffer
    memory_zero(console_buffer->memory, console_buffer->height*console_buffer->width);
    string_lines = string_split(&app_state->scratch_arena, app_state->string, '\n');
    Assert(string_lines.count > 0);
    Vec2i line_start_cursor = {0, app_state->view_y};
    U8 *string_addr = str_cursor_to_addr(string_lines, line_start_cursor);
    U32 string_index = string_addr - app_state->string.data;
    for (U32 i=0; i < console_buffer->height; i++)
    {
        for (U32 j=0; j < console_buffer->width; j++)
        {
            U32 console_buffer_index = i*console_buffer->width + j;
            if (string_index < app_state->string.size)
            {
                U8 next_letter = app_state->string.data[string_index++];
                U8 peek_letter = 0;
                if (string_index < app_state->string.size)
                {
                    peek_letter = app_state->string.data[string_index];
                }
                if (next_letter == '\n')
                {
                    break;
                }
                else
                {
                    console_buffer->memory[console_buffer_index] = next_letter;
                }
            }
            else
            {
                console_buffer->memory[console_buffer_index] = 0;
            }
        }
    }

    if (keyboard.keys[4].is_down)
    {
        console_buffer->memory[0] = 'a';
    }

    // NOTE: Cursor rendering
    // - Cursor XY -> String XY -> Screen XY
    string_cursor = get_string_cursor(string_lines, app_state->base_cursor);
    Vec2i view_cursor = {string_cursor.x, string_cursor.y - app_state->view_y};
    Assert(view_cursor.x < console_buffer->width);
    Assert(view_cursor.y < console_buffer->height);
    // TODO: This will break upon text wrapping
    console_buffer->cursor = view_cursor;
    // TODO: Set cursor type depending on insert or visual mode

    // Clear the scratch arena
    app_state->scratch_arena.pos = 0;
    app_state->last_keyboard = keyboard;
}