#include "string.h"

////////////////////////
// NOTE: String Functions

function B32
is_whitespace(U8 c)
{
    B32 result = 0;
    if (c == ' ' || c == '\n' ||  c == '\t')
    {
        result = 1;
    }
    return result;
}

// NOTE: Returns T/F on if the char is found
// NOTE: n starts at 0
function U32
string_find_n(String string, U8 c, U32 n, U32 *index_out)
{
    U32 result = 0;
    U32 counter = 0;
    for (U32 i=0; i < string.size; i++)
    {
        if (string.data[i] == c)
        {
            counter += 1;
            if (counter-1 == n)
            {
                result = 1;
                *index_out = i;
                break;
            }
        }
    }
    return result;
}

// NOTE: end is exclusive
function String
string_slice(String string, U32 start, U32 end)
{
    String result = {0};
    result.data = string.data + start;
    result.size = end - start;
    return result;
}

function B32
string_compare(String a, String b)
{
    B32 result = 1;
    if (a.size == b.size)
    {
        for (U32 i=0; i < a.size; i++)
        {
            if (a.data[i] != b.data[i])
            {
                result = 0;
                break;
            }
        }
    }
    else
    {
        result = 0;
    }
    return result;
}

// NOTE: This doesnt allocate new string copies, it keeps the underlying string memory
function StringArray
string_split(Arena *arena, String string, U8 c)
{
    StringArray result = {0};

    // Need to allocate the first part
    result.strings = push_struct(arena, String);
    U32 last_index = 0;
    for (U32 i=0; i < string.size; i++)
    {
        if (string.data[i] == c)
        {
            // NOTE: i-last_index-1 cuz we dont wanna count the replace char
            result.strings[result.count++] = (String){string.data+last_index, i-last_index};
            last_index = i+1;
            // Allocate space for the next part
            push_struct(arena, String);
        }
    }
    // Put in the rest
    result.strings[result.count++] = (String){string.data+last_index, string.size-last_index};
    Assert(result.count > 0);

    return result;
}

// NOTE: Reallocates the string
// Returns success/failure
function void
string_insert_char(Arena *arena, String *string_inout, U32 index, U8 c)
{
    // TODO: Theres gotta be a better way to do this other than bumping the arena pos
    // TODO: Also the string max size is whatever the string_arena size is
    Assert(arena->pos < arena->cap);
    arena->pos += 1;
    if (index < string_inout->size)
    {
        U8 *src = string_inout->data + index;
        U8 *dst = string_inout->data + index + 1;
        U32 n = string_inout->size - index;
        memory_copy(dst, src, n);
    }
    string_inout->data[index] = c;
    string_inout->size += 1;
}

function void
string_delete_range(Arena *arena, String *string_inout, U32 range_start, U32 range_end)
{
    Assert(range_start < string_inout->size);
    Assert(range_end <= string_inout->size);
    U8 *src = string_inout->data + range_end;
    U8 *dst = string_inout->data + range_start;
    memory_zero(dst, range_end-range_start);
    U32 n = string_inout->size - range_end;
    memory_copy(dst, src, n);
    string_inout->size -= range_end-range_start;
    arena->pos -= range_end-range_start;
}

function void
string_remove_index(Arena *arena, String *string_inout, U32 index)
{
    Assert(index < string_inout->size);
    U8 *src = string_inout->data + index + 1;
    U8 *dst = string_inout->data + index;
    U32 n = string_inout->size - index - 1;
    memory_copy(dst, src, n);
    string_inout->size -= 1;
    arena->pos -= 1;
}

// If n > 0, then just match the first n chars
function B32
string_match(String a, String b, U32 n)
{    B32 result = 0;
    if (((n == 0) && (a.size == b.size)) ||
        ((n > 0) && (a.size >= n) && (b.size >= n)))
    {
        result = 1;
        U32 cmp_len = a.size;
        if (n > 0)
        {
            cmp_len = n;
        }
        for (U64 i = 0; i < cmp_len; i += 1)
        {
            U8 ac = a.data[i];
            U8 bc = b.data[i];
            if (ac != bc)
            {
                result = 0;
                break;
            }
        }
    }
    return result;
}

// TODO: We can refactor this to use a StringJoin, keep the underlying memory,
//       then render out a new string at the very end
function String
string_replace(Arena *arena, String s, String find_string, String replace_string)
{
    String result = {(U8*)arena->base+arena->pos, 0};

    // NOTE: Is it bad to slowly append to an arena 1 by 1
    for (U32 i=0; i < s.size; i++)
    {
        String cur_string = {&s.data[i], s.size-i};
        U32 chars_left = s.size-i;
        if ((chars_left >= find_string.size) && string_match(cur_string, find_string, find_string.size))
        {
            // Replace the string
            U8 *copy_pos = (U8*)arena_push_size(arena, replace_string.size);
            memory_copy(copy_pos, replace_string.data, replace_string.size);
            // NOTE: Subtract 1 here cuz i will be incremented in the for loop
            i += find_string.size-1;
            result.size += replace_string.size;
        }
        else
        {
            U8 *c = (U8*)arena_push_size(arena, sizeof(U8));
            *c = s.data[i];
            result.size += 1;
        }
    }

    return result;
}
