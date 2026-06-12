#ifndef LEXER_H
#define LEXER_H

#define STANDALONE_TOKEN_LIST \
    X(TOKEN_LPAREN, "(") \
    X(TOKEN_RPAREN, ")") \
    X(TOKEN_LBRACE, "{") \
    X(TOKEN_RBRACE, "}") \
    X(TOKEN_LBRACKET, "[") \
    X(TOKEN_RBRACKET, "]") \
    X(TOKEN_APOSTRAPHE, "'") \
    X(TOKEN_QUOTE, "\"") \
    X(TOKEN_BACKSLASH, "\\") \
    X(TOKEN_AT, "@") \
    X(TOKEN_COMMA, ",") \
    X(TOKEN_DOT, ".") \
    X(TOKEN_MINUS, "-") \
    X(TOKEN_PLUS, "+") \
    X(TOKEN_STAR, "*") \
    X(TOKEN_SLASH, "/") \
    X(TOKEN_COLON, ":") \
    X(TOKEN_SEMICOLON, ";") \
    X(TOKEN_DOLLAR, "$") \
    X(TOKEN_BW_AND, "&") \
    X(TOKEN_BW_OR, "|") \
    X(TOKEN_BW_XOR, "^") \
    X(TOKEN_BW_NOT, "~") \
    X(TOKEN_BACKTICK, "`") \
    X(TOKEN_PERCENT, "%") \
    X(TOKEN_QUESTION, "?") \
    X(TOKEN_POUND, "#") \
    X(TOKEN_MINUS_MINUS, "--") \
    X(TOKEN_PLUS_PLUS, "++") \
    X(TOKEN_BANG, "!") \
    X(TOKEN_BANG_EQUAL, "!=") \
    X(TOKEN_EQUAL, "=") \
    X(TOKEN_EQUAL_EQUAL, "==") \
    X(TOKEN_GREATER, ">") \
    X(TOKEN_GREATER_EQUAL, ">=") \
    X(TOKEN_LESS, "<") \
    X(TOKEN_LESS_EQUAL, "<=") \
    X(TOKEN_AND, "&&") \
    X(TOKEN_OR, "||") \
    X(TOKEN_LSHIFT, "<<") \
    X(TOKEN_RSHIFT, ">>") \
    X(TOKEN_STAR_EQUAL, "*=") \
    X(TOKEN_SLASH_EQUAL, "/=") \
    X(TOKEN_PERCENT_EQUAL, "%=") \
    X(TOKEN_PLUS_EQUAL, "+=") \
    X(TOKEN_MINUS_EQUAL, "-=") \
    X(TOKEN_LSHIFT_EQUAL, "<<=") \
    X(TOKEN_RSHIFT_EQUAL, ">>=") \
    X(TOKEN_BW_AND_EQUAL, "&=") \
    X(TOKEN_BW_XOR_EQUAL, "^=") \
    X(TOKEN_BW_OR_EQUAL, "|=") \
    X(TOKEN_ELLIPSES, "...") 


typedef enum 
{
    TOKEN_TYPE_NULL, 

#define X(Enum, CString) Enum,
    STANDALONE_TOKEN_LIST
#undef X
    STANDALONE_TOKEN_MARKER,

    TOKEN_TYPE_STRING, 
    TOKEN_TYPE_IDENTIFIER, 
    TOKEN_TYPE_CONSTANT, 

    TOKEN_TYPE_COUNT, 
} Token_Type;

#define STANDALONE_TOKEN_COUNT (STANDALONE_TOKEN_MARKER - TOKEN_TYPE_NULL - 1)

typedef struct 
{
    Token_Type type;
    String    value;
} Token;

typedef struct 
{
    Token *tokens;
    U32 count;
} TokenArray;


function TokenArray tokenize_string(Arena *arena, String string);

#endif