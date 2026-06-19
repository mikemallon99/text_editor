#include "types.c"
#include "string.c"
#include "lexer.c"
#include "font.c"

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
app_open_file(AppState *app_state, AppMemory *memory, Arena *arena, String filepath)
{
    // Read file
    // TODO: Need to make string memory persistent
    U8 *str_cpy_data = arena_push_size(arena, filepath.size+1);
    memory_copy(str_cpy_data, filepath.data, filepath.size);
    app_state->filepath = (String){str_cpy_data, filepath.size};
    String file_contents = read_file_to_string(memory->platform_read_file, &app_state->scratch_arena, app_state->filepath);
    // Convert file to internal string representation (CRLF -> LF)
    // TODO: remove the existing string
    app_state->string = string_replace(&app_state->string_arena, file_contents, str_lit("\r\n"), str_lit("\n"));
}

function U32
get_token_array_index(TokenArray token_array, U8 *string_cursor)
{
    U32 result = 0;
    B32 found = 0;
    for (U32 i=0; i < token_array.count; i++)
    {
        if (i == token_array.count-1)
        {
            result = i;
            found = 1;
            break;
        }
        else if (string_cursor >= token_array.tokens[i].value.data && 
                 string_cursor < token_array.tokens[i+1].value.data)
        {
            result = i;
            found = 1;
            break;
        }
    }
    Assert(found);
    return result;
}

function Token*
get_next_nonwhitespace_token(TokenArray token_array, U8 *cursor)
{
    Token *result;
    U32 index = get_token_array_index(token_array, cursor);
    index += 1;
    while (index < token_array.count)
    {
        Token *check_token = &(token_array.tokens[index]);
        if (check_token->type != TOKEN_TYPE_WHITESPACE)
        {
            result = check_token;
            break;
        }
        index += 1;
    }
    return result;
}

function Token*
get_prev_nonwhitespace_token(TokenArray token_array, U8 *cursor)
{
    Token *result;
    U32 index = get_token_array_index(token_array, cursor);
    // Jump to the start of current token if not already at the start
    if (token_array.tokens[index].type != TOKEN_TYPE_WHITESPACE &&
        cursor > token_array.tokens[index].value.data)
    {
        result = &(token_array.tokens[index]);
    }
    else
    {
        index -= 1;
        while (index >= 0)
        {
            Token *check_token = &(token_array.tokens[index]);
            if (check_token->type != TOKEN_TYPE_WHITESPACE)
            {
                result = check_token;
                break;
            }
            index -= 1;
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
        void *perm_arena_base = (void*)((U8*)memory->memory + sizeof(AppState));
        U32 perm_arena_size = MB(16);
        app_state->perm_arena = arena_make(perm_arena_base, perm_arena_size);

        void *scratch_arena_base = (void*)((U8*)perm_arena_base + perm_arena_size);
        U32 scratch_arena_size = MB(16);
        app_state->scratch_arena = arena_make(scratch_arena_base, scratch_arena_size);

        void *string_arena_base = (void*)((U8*)scratch_arena_base + scratch_arena_size);
        U32 string_arena_size = MB(16);
        app_state->string_arena = arena_make(string_arena_base, string_arena_size);

        String cli_input_filepath = (String){memory->cli_input_filepath, strlen(memory->cli_input_filepath)};
        app_open_file(app_state, memory, &app_state->perm_arena, cli_input_filepath);

        app_state->last_keyboard = keyboard;
        app_state->buffer_view_dims.x = console_buffer->width;
        app_state->buffer_view_dims.y = console_buffer->height-2;

        // TODO: Eventually move font init to renderer component
        // Load the font
        String bmp_contents = read_file_to_string(memory->platform_read_file, &app_state->scratch_arena, str_lit("w:/text_editor/data/font.bmp"));
        BMPFile bmp_file = read_bmp_file(&app_state->scratch_arena, bmp_contents);
        app_state->font = read_bmp_font(&app_state->perm_arena, bmp_file);

        app_state->is_initialized = 1;
    }

    // TODO:
    // - Need visual way to debug input gathering
    // - undo history
    // - e/y scrolling
    // - proper handling of "chords" (ci", cw)
    // - Lock framerate to 60 (crash if <60 with error report)
    // - Crashes when line is > 120 chars
    // - nullptr token crash
    // - real 'visual mode' with cursor
    // - filesystem stuff
    //   - relative file opens from current directory
    //   - multiple buffers
    //   - file system explorer

    // NOTE: Cursor XY needed for memory on hugging line end
    // Cursor XY -> String XY
    StringArray string_lines = string_split(&app_state->scratch_arena, app_state->string, '\n');
    Vec2i string_cursor = get_string_cursor(string_lines, app_state->base_cursor);

    // NOTE: Tokenize everything for vim motions
    TokenArray token_array = tokenize_string(&app_state->scratch_arena, app_state->string);
    U8 *string_data_ptr = str_cursor_to_addr(string_lines, string_cursor);

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

        if ((keyboard.keys[keycode].is_down && !app_state->last_keyboard.keys[keycode].is_down) ||
            (app_state->frames_key_down[keycode] > 60 && app_state->frames_key_down[keycode] % 4 == 0))
        {
            if (app_state->input_mode == INPUTMODE_NORMAL && keycode < KEY_RETURN)
            {
                app_state->input_stack[app_state->input_stack_top++] = keycode_to_char(keycode, keyboard.keys[KEY_SHIFT].is_down);
            }
            if (app_state->input_mode == INPUTMODE_NORMAL && keycode == KEY_BACKSPACE && app_state->input_stack_top)
            {
                app_state->input_stack_top -= 1;
            }
            // Handle running a command
            else if (app_state->input_mode == INPUTMODE_NORMAL && keycode == KEY_RETURN && 
                     app_state->input_stack_top && app_state->input_stack[0] == ':')
            {
                String raw_cmd = (String){&app_state->input_stack[1], app_state->input_stack_top-1};
                StringArray cmd_split = string_split(&app_state->scratch_arena, raw_cmd, ' ');
                Assert(cmd_split.count > 0);
                String cmd = cmd_split.strings[0];
                StringArray cmd_args = {0};
                if (cmd_split.count > 1)
                {
                    cmd_args.count = cmd_split.count - 1;
                    cmd_args.strings = &cmd_split.strings[1];
                }
                if (string_compare(cmd, str_lit("w")))
                {
                    // Save file
                    memory->platform_write_file(app_state->filepath.data, app_state->string.data, app_state->string.size);
                    app_state->input_stack_top = 0;
                    app_state->message = str_lit("Saved!");
                    app_state->message_timer = 60;
                }
                else if (string_compare(cmd, str_lit("e")))
                {
                    if (cmd_args.count == 1)
                    {
                        app_open_file(app_state, memory, &app_state->perm_arena, cmd_args.strings[0]);
                    }
                    else
                    {
                        app_state->message = str_lit("Incorrect args to ':e <file>'");
                        app_state->message_timer = 60;
                    }
                    app_state->input_stack_top = 0;
                }
                else
                {
                    app_state->input_stack_top = 0;
                    app_state->message = str_lit("Command not found");
                    app_state->message_timer = 60;
                }
                app_state->input_stack_top = 0;
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
                    app_state->input_mode = INPUTMODE_NORMAL;
                }
            }
        }
    }

    // TODO: Need better ways to destroy the input stack
    //       Like some kind of stack terminating chars (numbers)
    if (keyboard.keys[KEY_ESC].is_down)
    {
        app_state->input_stack_top = 0;
    }

    if ((app_state->input_mode == INPUTMODE_NORMAL) && app_state->input_stack_top)
    {
        String stack_string = (String){app_state->input_stack, app_state->input_stack_top};
        // TODO: Need better system for this
        // Just ignore the input stack if its a command
        if (stack_string.data[0] == ':')
        {
        }
        // Left
        else if (string_compare(stack_string, str_lit("h")))
        {
            // NOTE: Dont go up to previous line if you hit left
            if (string_cursor.x > 0)
            {
                app_state->base_cursor.x = string_cursor.x - 1;
            }
            app_state->input_stack_top = 0;
        }
        // Right
        else if (string_compare(stack_string, str_lit("l")))
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
        else if (string_compare(stack_string, str_lit("k")))
        {
            if (string_cursor.y > 0)
            {
                app_state->base_cursor.y = string_cursor.y - 1;
            }
            app_state->input_stack_top = 0;
        }
        // Down
        else if (string_compare(stack_string, str_lit("j")))
        {
            // Go to the next line at the x value below
            if (string_cursor.y < string_lines.count-1)
            {
                app_state->base_cursor.y = string_cursor.y + 1;
            }
            app_state->input_stack_top = 0;
        }
        // Insert
        else if (string_compare(stack_string, str_lit("i")))
        {
            app_state->input_mode = INPUTMODE_INSERT;
            app_state->input_stack_top = 0;
        }
        ////////////////////////////
        // MOTIONS
        // Start/end of line
        else if (string_compare(stack_string, str_lit("0")))
        {
            app_state->base_cursor.x = 0;
            app_state->input_stack_top = 0;
        }
        else if (string_compare(stack_string, str_lit("$")))
        {
            // TODO: Sticks to EOL unintentionally (even tho this matches Vim)
            String string_line = string_lines.strings[string_cursor.y];
            app_state->base_cursor.x = string_line.size-1;
            app_state->input_stack_top = 0;
        }
        // Start/end of file
        else if (string_compare(stack_string, str_lit("gg")))
        {
            app_state->base_cursor.x = 0;
            app_state->base_cursor.y = 0;
            app_state->input_stack_top = 0;
        }
        else if (string_compare(stack_string, str_lit("G")))
        {
            app_state->base_cursor.x = 0;
            app_state->base_cursor.y = string_lines.count-1;
            app_state->input_stack_top = 0;
        }
        // Delete line
        else if (string_compare(stack_string, str_lit("dd")))
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
        else if (string_compare(stack_string, str_lit("D")))
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
        else if (string_compare(stack_string, str_lit("C")))
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
        // Change word
        else if (string_compare(stack_string, str_lit("cw")))
        {
            // Go to start of next token
            U8 *cursor_addr = str_cursor_to_addr(string_lines, string_cursor);
            Token *next_token = get_next_nonwhitespace_token(token_array, cursor_addr);
            if (next_token)
            {
                U8 *range_start_addr = str_cursor_to_addr(string_lines, string_cursor);
                U8 *range_end_addr = next_token->value.data;
                U32 range_start = range_start_addr - app_state->string.data;
                U32 range_end = range_end_addr - app_state->string.data;
                string_delete_range(&app_state->string_arena, 
                                    &app_state->string, 
                                    range_start, 
                                    range_end);
            }
            app_state->input_mode = INPUTMODE_INSERT;
            app_state->input_stack_top = 0;
        }
        else if (string_compare(stack_string, str_lit("ci)")))
        {
            // Find the token range
            U32 token_array_index = get_token_array_index(token_array, string_data_ptr);
            Token *right;
            U32 right_index;
            for (U32 i=token_array_index; i < token_array.count; i++)
            {
                if (token_array.tokens[i].type == TOKEN_RPAREN)
                {
                    right = &(token_array.tokens[i]);
                    right_index = i;
                    break;
                }
            }
            Token *left;
            if (right)
            {
                // TODO: Push/popping
                U32 rparen_stack = 0;
                for (S32 i=right_index; i >= 0; i--)
                {
                    if (token_array.tokens[i].type == TOKEN_RPAREN)
                    {
                        rparen_stack += 1;
                    }
                    if (token_array.tokens[i].type == TOKEN_LPAREN)
                    {
                        rparen_stack -= 1;
                        if (rparen_stack == 0)
                        {
                            left = &(token_array.tokens[i]);
                            break;
                        }
                    }
                }
            }
            if (left && right)
            {
                // Delete the range
                U8 *range_start_addr = left->value.data + 1;
                U8 *range_end_addr = right->value.data;
                U32 range_start = range_start_addr - app_state->string.data;
                U32 range_end = range_end_addr - app_state->string.data;
                app_state->base_cursor = str_addr_to_cursor(string_lines, range_start_addr);
                string_delete_range(&app_state->string_arena, 
                                    &app_state->string, 
                                    range_start, 
                                    range_end);
                app_state->input_mode = INPUTMODE_INSERT;
            }
            app_state->input_stack_top = 0;
        }
        // Next/prev word
        else if (string_compare(stack_string, str_lit("w")))
        {
            // Go to start of next token
            U8 *cursor_addr = str_cursor_to_addr(string_lines, string_cursor);
            Token *next_token = get_next_nonwhitespace_token(token_array, cursor_addr);
            if (next_token)
            {
                U8 *token_start_ptr = next_token->value.data;
                app_state->base_cursor = str_addr_to_cursor(string_lines, token_start_ptr);
            }
            app_state->input_stack_top = 0;
        }
        else if (string_compare(stack_string, str_lit("b")))
        {
            // Go to start of current or previous token
            U8 *string_addr = str_cursor_to_addr(string_lines, string_cursor);
            Token *prev_token = get_prev_nonwhitespace_token(token_array, string_addr);
            if (prev_token)
            {
                U8 *token_start_ptr = prev_token->value.data;
                app_state->base_cursor = str_addr_to_cursor(string_lines, token_start_ptr);
            }
            app_state->input_stack_top = 0;
        }
        // Up/down half a page
        else if (keyboard.keys[KEY_CTRL].is_down && string_compare(stack_string, str_lit("d")))
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
        else if (keyboard.keys[KEY_CTRL].is_down && string_compare(stack_string, str_lit("u")))
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
    }

    // Update view based on latest cursor
    U32 view_bottom = app_state->view_y + app_state->buffer_view_dims.y;
    if (app_state->base_cursor.y < app_state->view_y)
    {
        app_state->view_y = app_state->base_cursor.y;
    }
    else if (app_state->base_cursor.y >= view_bottom)
    {
        app_state->view_y = (app_state->base_cursor.y + 1) - app_state->buffer_view_dims.y;
    }

    if (app_state->message_timer > 0)
    {
        app_state->message_timer -= 1;
        if (app_state->message_timer == 0)
        {
            app_state->message.data = 0;
            app_state->message.size = 0;
        }
    }

    // Render

    // NOTE: Internally store a string interface, render as XY buffer
    memory_zero(console_buffer->memory, console_buffer->height*console_buffer->width);
    string_lines = string_split(&app_state->scratch_arena, app_state->string, '\n');
    Assert(string_lines.count > 0);
    Vec2i line_start_cursor = {0, app_state->view_y};
    U8 *string_addr = str_cursor_to_addr(string_lines, line_start_cursor);
    U32 string_index = string_addr - app_state->string.data;
    for (U32 i=0; i < app_state->buffer_view_dims.y; i++)
    {
        for (U32 j=0; j < app_state->buffer_view_dims.x; j++)
        {
            U32 console_buffer_index = i*app_state->buffer_view_dims.x + j;
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

    // NOTE: Toolbar rendering
    // Mode
    U32 toolbar_row = console_buffer->height - 2;
    U8 *toolbar_memory = &console_buffer->memory[toolbar_row*console_buffer->width];
    memory_set(toolbar_memory, '-', console_buffer->width);
    if (app_state->input_mode == INPUTMODE_INSERT)
    {
        memory_copy(toolbar_memory, "INSERT", 6);
    }
    else if (app_state->input_mode == INPUTMODE_NORMAL)
    {
        memory_copy(toolbar_memory, "NORMAL", 6);
    }
    else if (app_state->input_mode == INPUTMODE_VISUAL)
    {
        memory_copy(toolbar_memory, "VISUAL", 6);
    }
    else
    {
        Assert(0);
    }

    // Filename
    U32 filepath_offset = console_buffer->width - app_state->filepath.size;
    memory_copy(toolbar_memory+filepath_offset, app_state->filepath.data, app_state->filepath.size);

    // Message
    U32 message_offset = 10;
    memory_copy(toolbar_memory+message_offset, app_state->message.data, app_state->message.size);

    // Command bar
    if (app_state->input_stack_top && app_state->input_stack[0] == ':')
    {
        U32 commandbar_row = console_buffer->height - 1;
        U8 *commandbar_memory = &console_buffer->memory[commandbar_row*console_buffer->width];
        memory_copy(commandbar_memory, app_state->input_stack, app_state->input_stack_top);
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