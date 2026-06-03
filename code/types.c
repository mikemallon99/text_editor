#include "types.h"

void *
arena_push_size(Arena *arena, U32 size)
{
    Assert((arena->pos + size) <= arena->cap);
    void *result = (void *)((U8*)(arena->base) + arena->pos);
    // TODO: memset to 0 here?
    arena->pos += size;
    return result;
}

function Arena
arena_make(void *memory, U32 size)
{
    Arena result = {0};
    result.base = memory;
    result.cap = size;
    return result;
}