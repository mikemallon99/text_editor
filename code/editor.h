#ifndef EDITOR_H
#define EDITOR_H

////////////////////////
// NOTE: App <-> Platform Stuff

typedef struct 
{
    U32 width;
    U32 height;
    U8 *memory;
    Vec2i cursor;
} ConsoleBuffer;

typedef enum 
{
    KEY_NULL,
    KEY_UPARROW,
    KEY_DOWNARROW,
    KEY_LEFTARROW,
    KEY_RIGHTARROW,
    KEY_RETURN,
    KEY_CTRL,
    KEY_ALT,
    KEY_ESC,
    KEY_BACKSPACE,
    KEY_SHIFT,
    KEY_TAB,
    KEY_CHAR,
} KeyType;

typedef struct 
{
    KeyType key_type;
    U8 key_value;
} Input;

#define PLATFORM_READ_FILE(name) U32 name(U8 *file_path, U8 *buffer, U32 buffer_size, U32 *file_size_out)
typedef PLATFORM_READ_FILE(PlatformReadFileFunc);

typedef struct 
{
    void *memory;
    U32   size;
    PlatformReadFileFunc *platform_read_file;
    U8 *cli_input_filepath;
} AppMemory;

////////////////////////
// NOTE: App Stuff

typedef enum
{
    INPUTMODE_VISUAL,
    INPUTMODE_INSERT,
} InputMode;

typedef struct 
{
    B32 is_initialized;
    // Gets cleared at the end of each frame
    Arena scratch_arena;
    // Stores the underlying string, dont put anything else on this
    Arena string_arena;
    // Underlying string that youre editing
    String string;

    Vec2i base_cursor;
    InputMode input_mode;
    U32 view_y;

    // NOTE: Used for combos like "ci)" and "dd"
    U8 input_stack[8];
    U32 input_stack_top;
} AppState;

#endif