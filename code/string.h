#ifndef STRING_H
#define STRING_H

////////////////////////
// NOTE: String Types

typedef struct
{
    U8 *data;
    U32 size;
} String;

typedef struct
{
    String *strings;
    U32 count;
} StringArray;

#define str_lit(s) (String){s, strlen(s)}

////////////////////////
// NOTE: String Functions

function B32 is_whitespace(U8 c);

function void string_insert_char(Arena *arena, String *string_inout, U32 index, U8 c);
function void string_remove_index(Arena *arena, String *string_inout, U32 index);
function B32  string_compare(String a, String b);
function B32  string_match(String a, String b, U32 n);
function U32  string_find_n(String string, U8 c, U32 n, U32 *index_out);
function String string_slice(String string, U32 start, U32 end);
function String string_replace(Arena *arena, String s, String find_string, String replace_string);
function StringArray string_split(Arena *arena, String string, U8 c);

#endif