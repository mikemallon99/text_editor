#include "types.c"
#include "string.c"
#include "lexer.c"

#include "editor.h"

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

function U32
get_string_index(StringArray string_lines, Vec2i string_cursor)
{
    U32 result = 0;
    for (U32 i=0; i < string_cursor.y; i++)
    {
        // NOTE: +1 cuz the newlines arent in the string split
        result += string_lines.strings[i].size+1;
    }
    result += string_cursor.x;
    return result;
}

function void
app_update(AppMemory *memory, Input input, ConsoleBuffer *console_buffer)
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
        String filepath_string = {memory->cli_input_filepath, strlen(memory->cli_input_filepath)};
        String file_contents = read_file_to_string(memory->platform_read_file, &app_state->scratch_arena, filepath_string);

        // Convert file to internal string representation (CRLF -> LF)
        app_state->string = string_replace(&app_state->string_arena, file_contents, str_lit("\r\n"), str_lit("\n"));

        app_state->is_initialized = 1;
    }

    // TODO: Actually use this token array for jumping around with motions
    TokenArray token_array = tokenize_string(&app_state->scratch_arena, app_state->string);

    // Insert Mode vs Visual Mode
    // Inputs + Update
    if (input.key_type)
    {
        // CURSOR NAVIGATION:
        // - Cursor XY -> String XY -> String Index 
        // - Cursor XY is needed to have memory for staying on EOL

        // Cursor XY -> String XY
        // NOTE: Save arena pos to pop at the end of this scope
        StringArray string_lines = string_split(&app_state->scratch_arena, app_state->string, '\n');
        Assert(string_lines.count > 0);
        Vec2i string_cursor = get_string_cursor(string_lines, app_state->base_cursor);

        if (app_state->input_mode == INPUTMODE_VISUAL)
        {
            // Left
            if (input.key_type == KEY_CHAR && input.key_value == 'h')
            {
                // NOTE: Dont go up to previous line if you hit left
                if (string_cursor.x > 0)
                {
                    app_state->base_cursor.x = string_cursor.x - 1;
                }
            }
            // Right
            else if (input.key_type == KEY_CHAR && input.key_value == 'l')
            {
                // NOTE: Notice that were calculating off the string XY, not the cursor XY
                String string_line = string_lines.strings[string_cursor.y];
                // NOTE: string_line.size-1, because visual mode can only reach the last char
                if (string_cursor.x < string_line.size-1)
                {
                    app_state->base_cursor.x = string_cursor.x + 1;
                }
            }
            // Up
            else if (input.key_type == KEY_CHAR && input.key_value == 'k')
            {
                if (string_cursor.y > 0)
                {
                    app_state->base_cursor.y = string_cursor.y - 1;
                }
            }
            // Down
            else if (input.key_type == KEY_CHAR && input.key_value == 'j')
            {
                // Go to the next line at the x value below
                if (string_cursor.y < string_lines.count-1)
                {
                    app_state->base_cursor.y = string_cursor.y + 1;
                }
            }
            // Insert
            else if (input.key_type == KEY_CHAR && input.key_value == 'i')
            {
                app_state->input_mode = INPUTMODE_INSERT;
            }
            ////////////////////////////
            // MOTIONS
            // Start/end of line
            else if (input.key_type == KEY_CHAR && input.key_value == '0')
            {
                app_state->base_cursor.x = 0;
            }
            else if (input.key_type == KEY_CHAR && input.key_value == '$')
            {
                // TODO: Sticks to EOL unintentionally (even tho this matches Vim)
                String string_line = string_lines.strings[string_cursor.y];
                app_state->base_cursor.x = string_line.size-1;
            }
            // Start/end of file
            else if (input.key_type == KEY_CHAR && input.key_value == 'g')
            {
                // TODO: I have a feeling ill need to handle the input stack in a more organized way
                if (app_state->input_stack_top == 1 && app_state->input_stack[0] == 'g')
                {
                    app_state->base_cursor.x = 0;
                    app_state->base_cursor.y = 0;
                    app_state->input_stack_top = 0;
                }
                else 
                {
                    app_state->input_stack[app_state->input_stack_top++] = 'g';
                }
            }
            else if (input.key_type == KEY_CHAR && input.key_value == 'G')
            {
                app_state->base_cursor.x = 0;
                app_state->base_cursor.y = string_lines.count-1;
            }
            // Next/prev word
            else if (input.key_type == KEY_CHAR && input.key_value == 'w')
            {
                // Go until youve passed the next group of whitespaces
                U32 string_index = get_string_index(string_lines, string_cursor);
                B32 found_whitespace = is_whitespace(app_state->string.data[string_index]);
                while (1)
                {
                    string_cursor.x += 1;
                    string_index = get_string_index(string_lines, string_cursor);
                    if (!found_whitespace)
                    {
                        // Increment until first whitespace is found
                        found_whitespace = is_whitespace(app_state->string.data[string_index]);
                    }
                    else
                    {
                        // Once first whitespace is found, increment until first non whitespace is found
                        if (!is_whitespace(app_state->string.data[string_index]))
                        {
                            break;
                        }
                    }
                }
                app_state->base_cursor.x = string_cursor.x;
            }
        }
        else if (app_state->input_mode == INPUTMODE_INSERT)
        {
            if (input.key_type == KEY_CHAR)
            {
                U32 string_index = get_string_index(string_lines, string_cursor);
                // NOTE: Cursor puts at current position and then increments
                string_insert_char(&app_state->string_arena, 
                                   &app_state->string, 
                                   string_index, 
                                   input.key_value);
                app_state->base_cursor.x += 1;
            }
            else if (input.key_type == KEY_RETURN)
            {
                U32 string_index = get_string_index(string_lines, string_cursor);
                string_insert_char(&app_state->string_arena, 
                                   &app_state->string, 
                                   string_index, 
                                   '\n');
                app_state->base_cursor.x = 0;
                app_state->base_cursor.y += 1;
            }
            else if (input.key_type == KEY_BACKSPACE)
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
                U32 string_index = get_string_index(string_lines, backspace_cursor);
                string_remove_index(&app_state->string_arena, 
                                    &app_state->string, 
                                    string_index);
                app_state->base_cursor = backspace_cursor;
            }
            else if (input.key_type == KEY_ESC)
            {
                app_state->input_mode = INPUTMODE_VISUAL;
            }
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
    // TODO: Should find a better way to pop off the scratch arena
    StringArray string_lines = string_split(&app_state->scratch_arena, app_state->string, '\n');
    Assert(string_lines.count > 0);
    Vec2i line_start_cursor = {0, app_state->view_y};
    U32 string_index = get_string_index(string_lines, line_start_cursor);
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


    // NOTE: Cursor rendering
    // - Cursor XY -> String XY -> Screen XY
    Vec2i string_cursor = get_string_cursor(string_lines, app_state->base_cursor);
    Vec2i view_cursor = {string_cursor.x, string_cursor.y - app_state->view_y};
    Assert(view_cursor.x < console_buffer->width);
    Assert(view_cursor.y < console_buffer->height);
    // TODO: This will break upon text wrapping
    console_buffer->cursor = view_cursor;
    // TODO: Set cursor type depending on insert or visual mode

    // Clear the scratch arena
    app_state->scratch_arena.pos = 0;
}