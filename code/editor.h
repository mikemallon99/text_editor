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
    // 0 - 9 == '0-9'
    // 10 - 35 == 'a-z'
    // 36 - 47 == other chars
    KEY_SEMICOLON = 36,
    KEY_PLUS,
    KEY_COMMA,
    KEY_MINUS,
    KEY_PERIOD,
    KEY_SLASH,
    KEY_BACKTICK,
    KEY_LBRACE,
    KEY_BACKSLASH,
    KEY_RBRACE,
    KEY_QUOTE,
    KEY_SPACE,
    // 48 - 53 == buttons
    KEY_RETURN,
    KEY_ESC,
    KEY_BACKSPACE,
    KEY_CTRL,
    KEY_ALT,
    KEY_SHIFT,
    KEY_TAB,

    KEY_COUNT
} KeyType;

typedef struct 
{
    B32 is_down;
    U32 half_trans_count;
} button_state;


typedef struct 
{
    // 0-9 & a-z & -=[];',./\`
    button_state keys[KEY_COUNT];
} Keyboard;

#define PLATFORM_READ_FILE(name) U32 name(U8 *file_path, U8 *buffer, U32 buffer_size, U32 *file_size_out)
typedef PLATFORM_READ_FILE(PlatformReadFileFunc);

#define PLATFORM_WRITE_FILE(name) void name(U8 *file_path, U8 *buffer, U32 buffer_size)
typedef PLATFORM_WRITE_FILE(PlatformWriteFileFunc);

typedef struct 
{
    void *memory;
    U32   size;
    PlatformReadFileFunc  *platform_read_file;
    PlatformWriteFileFunc *platform_write_file;
    U8 *cli_input_filepath;
} AppMemory;

typedef struct 
{
    void *memory;
    U32   width;
    U32   height;
    U32   pitch;
    U32   bytes_per_pixel;
} VideoMemory;

////////////////////////
// NOTE: App Stuff

typedef enum
{
    INPUTMODE_NORMAL,
    INPUTMODE_INSERT,
    INPUTMODE_VISUAL,
} InputMode;

typedef struct 
{
    B32 is_initialized;
    // Need for font
    Arena perm_arena;
    // Gets cleared at the end of each frame
    Arena scratch_arena;
    // Stores the underlying string, dont put anything else on this
    Arena string_arena;
    // Underlying string that youre editing
    String string;

    Font font;
    Vec2i base_cursor;
    Vec2i buffer_view_dims;
    InputMode input_mode;
    U32 view_y;
    Keyboard last_keyboard;
    String filepath;
    U32 frames_key_down[KEY_COUNT];
    String message;
    U32 message_timer;

    // NOTE: Used for combos like "ci)" and "dd", and commands
    U8 input_stack[512];
    U32 input_stack_top;
} AppState;

#endif