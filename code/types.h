#ifndef TYPES_H
#define TYPES_H

////////////////////////
// NOTE: Basic Types

#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) ((x) << 30)
#define TB(x) ((x) << 40)

#define global    static
#define local     static
#define function  static

#include <stdint.h>
typedef int8_t  S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;
typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef S8  B8;
typedef S16 B16;
typedef S32 B32;
typedef S64 B64;
typedef float F32;
typedef double F64;

// NOTE: Assert here is capital A cuz idk how to do lowercase...
#include <assert.h>
#define Assert(stmt) assert(stmt)

#include <stdlib.h>
#define memory_zero(src, n) memset((void*)src, 0, n)
#define memory_copy(dst, src, n) memcpy((void*)dst, (void*)src, n)

#define array_count(a) (sizeof(a)/sizeof((a)[0]))

////////////////////////
// NOTE: Basic Types

typedef struct 
{
    S32 x;
    S32 y;
} Vec2i;

typedef struct 
{
    void *base;
    U32   pos;
    U32   cap;
} Arena;


function void* arena_push_size(Arena *arena, U32 size);
#define push_struct(a,T) (T*)arena_push_size(a, sizeof(T))
#define push_array(a,T,n) (T*)arena_push_size(a, (n)*sizeof(T))

function Arena arena_make(void *memory, U32 size);

#endif