
function void
write_pixel_value(U32 *src, U32 *dst)
{
    U32 AlphaMask = 0xFF000000;
    U32 Alpha = (*src & AlphaMask) >> 24;
    // No color data
    if (Alpha == 0)
    {
    }
    // All Color data
    else if (Alpha == 0xFF)
    {
        *dst = *src;
    }
    // Alpha blending
    else 
    {
        F32 AlphaRatio = (F32)Alpha / (F32)0xFF;

        // U32 BufferA = (0xFF000000 & *BufferPixel) >> 24;
        U32 DestR = (0xFF0000 & *dst) >> 16;
        U32 DestG = (0xFF00 & *dst) >> 8;
        U32 DestB = (0xFF & *dst) >> 0;

        // U32 ImageA = (0xFF000000 & *ImagePixel) >> 24;
        U32 SourceR = (0xFF0000 & *src) >> 16;
        U32 SourceG = (0xFF00 & *src) >> 8;
        U32 SourceB = (0xFF & *src) >> 0;

        U32 A = 0xFF;
        U32 R = (U32)(((F32)DestR * (1.0f - AlphaRatio)) + ((F32)SourceR * (AlphaRatio)));
        U32 G = (U32)(((F32)DestG * (1.0f - AlphaRatio)) + ((F32)SourceG * (AlphaRatio)));
        U32 B = (U32)(((F32)DestB * (1.0f - AlphaRatio)) + ((F32)SourceB * (AlphaRatio)));
        
        *src = (A << 24) | (R << 16) | (G << 8) | (B << 0);
    }
}

function void 
app_render(ConsoleBuffer console_buffer, VideoMemory video_memory, Font font)
{
    // for (U32 ascii_idx=ASCII_START_CHAR; ascii_idx < ASCII_COUNT; ascii_idx++)
    // {
    //     U32 *video_pixel = (U32*)video_memory.memory + row*font.height*video_memory.width + col*font.width;
    //     U8 ascii_char = (U8)console_buffer.memory[row*console_buffer.width + col];
    //     U32 *font_pixel = font.data + ascii_char*font.width*font.height;
    // }

    // For each letter in the console buffer, copy the char onto the screen
    for (U32 row=0; row < console_buffer.height; row++)
    {
        for (U32 col=0; col < console_buffer.width; col++)
        {
            U32 *video_pixel = (U32*)video_memory.memory + row*font.height*video_memory.width + col*font.width;
            U8 ascii_char = (U8)console_buffer.memory[row*console_buffer.width + col];
            U32 *font_pixel = font.data + ascii_char*font.width*font.height;
            for (U32 i=0; i < font.height; i++)
            {
                for (U32 j=0; j < font.width; j++)
                {
                    write_pixel_value(font_pixel, video_pixel+j);
                    font_pixel++;
                }
                video_pixel += video_memory.width;
            }
        }
    }
}