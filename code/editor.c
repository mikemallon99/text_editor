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


// TODO: Virtual cursor
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

    // TODO: Build up an input buffer
    //       Always read inputs from buffer. When read, it clears the buffer

    // Insert Mode vs Visual Mode
    // Inputs + Update
    if (input.key_type)
    {
        // CURSOR NAVIGATION:
        // - Cursor XY -> String XY -> String Index 
        // - Cursor XY is needed to have memory for staying on EOL

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
                // Go to start of next token
                if (token_index.next)
                {
                    U8 *token_start_ptr = token_index.next->value.data;
                    app_state->base_cursor = str_addr_to_cursor(string_lines, token_start_ptr);
                }
            }
            else if (input.key_type == KEY_CHAR && input.key_value == 'b')
            {
                // Go to start of current or previous token
                U8 *string_addr = str_cursor_to_addr(string_lines, string_cursor);
                U8 *token_start_ptr = 0;
                // Check if at the start of the current token
                if (token_index.token && (string_addr == token_index.token->value.data))
                {
                    token_start_ptr = token_index.prev->value.data;
                }
                else if (token_index.prev)
                {
                    token_start_ptr = token_index.prev->value.data;
                }
                app_state->base_cursor = str_addr_to_cursor(string_lines, token_start_ptr);
            }
        }
        else if (app_state->input_mode == INPUTMODE_INSERT)
        {
            if (input.key_type == KEY_CHAR)
            {
                U8 *string_addr = str_cursor_to_addr(string_lines, string_cursor);
                U32 string_index = string_addr - app_state->string.data;
                // NOTE: Cursor puts at current position and then increments
                string_insert_char(&app_state->string_arena, 
                                   &app_state->string, 
                                   string_index, 
                                   input.key_value);
                app_state->base_cursor.x += 1;
            }
            else if (input.key_type == KEY_RETURN)
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
                U8 *string_addr = str_cursor_to_addr(string_lines, backspace_cursor);
                U32 string_index = string_addr - app_state->string.data;
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
}