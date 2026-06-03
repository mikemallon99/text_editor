#include "lexer.h"

function B32
is_digit(U8 c)
{
    B32 result = 0;

    if (c >= '0' && c <= '9')
    {
        result = 1;
    }

    return result;
}

function B32
is_alpha(U8 c)
{
    B32 result = 0;

    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_')
    {
        result = 1;
    }

    return result;
}

function TokenArray
tokenize_string(Arena *arena, String string)
{
    TokenArray result = {0};
    result.tokens = (Token*)((U8*)arena->base + arena->pos);

    // NOTE: This X macro stuff creates lists of all our token types with their strings
    Token standalone_tokens[STANDALONE_TOKEN_COUNT] = 
    {
#define X(Enum, CString) {Enum, {CString, sizeof(CString)-1}},
        STANDALONE_TOKEN_LIST
#undef X
    };

    U32 cursor = 0;
    while (cursor < string.size)
    {
        while (is_whitespace(string.data[cursor]))
        {
            cursor++;
        }

        // Standalone First
        Token last_token = {0};
        for (U32 token_index = 0;
             token_index < array_count(standalone_tokens);
             token_index++)
        {
            // NOTE: This just crosses fingers and hopes its the longest rn
            // Take the longest match of the lot
            Token check_token = standalone_tokens[token_index];
            String temp_string = (String){string.data+cursor, check_token.value.size};
            if (string_compare(temp_string, check_token.value))
            {
                last_token = check_token;
            }
        }
        // NOTE: Take the last successful result, so we use "==" over "="
        if (last_token.type != TOKEN_TYPE_NULL)
        {
            cursor += last_token.value.size;
            // NOTE: Push token here, gotta be a better way to handle the arena part here tho
            Token *test = push_struct(arena, Token);
            result.tokens[result.count++] = last_token;
            continue;
        }
        else
        {
            // If its not a standalone token, then we just parse as an alphanumeric string
            // NOTE: All we need here is alpha numeric since its not for a full language
            U32 marker = cursor;
            while ((cursor < string.size) && 
                (is_alpha(string.data[cursor]) || is_digit(string.data[cursor])))
            {
                cursor += 1;
            }
            String alphanum_string = {&string.data[marker], cursor-marker};
            Token alphanum_token = {TOKEN_TYPE_IDENTIFIER, alphanum_string};
            push_struct(arena, Token);
            result.tokens[result.count++] = alphanum_token;
        }
    }

    return result;
}