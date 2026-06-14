typedef struct
{
    U32 width;
    U32 height;
    U32 bits_per_pixel;
    U32 image_size;
    U32* pixels;
} BMPFile;

function U32
get_bit_shift(U32 Value)
{
    if (Value == 0xFF) 
    {
        return 0;
    }
    else if (Value == 0xFF00) 
    {
        return 8;
    }
    else if (Value == 0xFF0000) 
    {
        return 16;
    }
    else if (Value == 0xFF000000) 
    {
        return 24;
    }
    return 0;
}

function BMPFile
read_bmp_file(Arena *arena, String bmp_file)
{
    BMPFile result = {0};
    U32 bmp_size = *((U32 *)(bmp_file.data + 2));
    U32 offset = *((U32 *)(bmp_file.data + 10));
    U32 *file_pixels = (U32 *)(bmp_file.data + offset);

    result.width = *((U32 *)(bmp_file.data + 18));
    result.height = *((U32 *)(bmp_file.data + 22));
    result.bits_per_pixel = *((U16 *)(bmp_file.data + 28));
    result.image_size = *((U32 *)(bmp_file.data + 34));
    U32 compression_method = *((U32 *)(bmp_file.data + 30));

    U32 transparent_color_1 = 0xFF747474;
    U32 transparent_color_2 = 0xFF008000;

    // NOTE: These values arent tested, ive only tested using the bitmask
    U32 red_mask = 0xFF000000;
    U32 green_mask = 0xFF0000;
    U32 blue_mask = 0xFF00;
    U32 alpha_mask = 0xFF;
    // TODO: Fix this copy paste or it will bite me in butthole
    if (result.bits_per_pixel == 32)
    {
        if (compression_method == 3)
        {
            red_mask = *((U32 *)(bmp_file.data + 0x36));
            green_mask = *((U32 *)(bmp_file.data + 0x3A));
            blue_mask = *((U32 *)(bmp_file.data + 0x3E));
            alpha_mask = *((U32 *)(bmp_file.data + 0x42));
        }

        U32 red_shift = get_bit_shift(red_mask);
        U32 green_shift = get_bit_shift(green_mask);
        U32 blue_shift = get_bit_shift(blue_mask);
        U32 alpha_shift = get_bit_shift(alpha_mask);

        U32 pixel_count = result.width*result.height;
        U32 *memory_pixels = push_array(arena, U32, pixel_count);
        result.pixels = memory_pixels;
        U32 *copy_pixel;
        // NOTE: It goes top to bottom, i wanna reverse it so the image pixels always starts at the top left corner
        for (U32 row_idx = result.height; 
            row_idx > 0; 
            row_idx--)
        {
            copy_pixel = file_pixels + result.width*(row_idx-1);
            for (U32 col_idx = 0; 
                col_idx < result.width; 
                col_idx++)
            {
                // NOTE: Should this pixel flipping happen at load or at draw ?
                U32 pixel = *copy_pixel++;
                U32 R = 0xFF & (pixel >> red_shift);
                U32 G = 0xFF & (pixel >> green_shift);
                U32 B = 0xFF & (pixel >> blue_shift);
                U32 A = 0xFF & (pixel >> alpha_shift);
                pixel = (A << 24) | (R << 16) | (G << 8) | (B);
                if (pixel == transparent_color_1 || pixel == transparent_color_2)
                {
                    pixel = 0;
                }
                *memory_pixels++ = pixel;
            }
        }
    }
    else if (result.bits_per_pixel == 24)
    {
        U32 pixel_count = result.width*result.height;
        U32 *memory_pixels = push_array(arena, U32, pixel_count);
        result.pixels = memory_pixels;
        U32 row_size = ((result.width * 3 + 3) / 4) * 4;
        U8 *copy_pixel;
        // NOTE: It goes top to bottom, i wanna reverse it so the image pixels always starts at the top left corner
        for (U32 row_idx = result.height; 
            row_idx > 0; 
            row_idx--)
        {
            copy_pixel = (U8 *)file_pixels + row_size*(row_idx-1);
            for (U32 col_idx = 0; 
                col_idx < result.width; 
                col_idx++)
            {
                // NOTE: Should this pixel flipping happen at load or at draw ?
                U32 B = 0xFF & (U32)*copy_pixel++;
                U32 G = 0xFF & (U32)*copy_pixel++;
                U32 R = 0xFF & (U32)*copy_pixel++;
                U32 A = 0xFF;
                U32 pixel = (A << 24) | (R << 16) | (G << 8) | (B);
                if (pixel == transparent_color_1 || pixel == transparent_color_2)
                {
                    pixel = 0;
                }
                *memory_pixels++ = pixel;
            }
        }
    }
    return result;
}

#define ASCII_COUNT 128
#define ASCII_CHAR_START 32

typedef struct
{
    U32 width;
    U32 height;
    U32 *data;
} Font;

function Font 
read_bmp_font(Arena *arena, BMPFile bmp_file)
{
    Font result = {0};
    // NOTE: assuming 5x7 font with single pixel padding and 128x64 image for now
    result.width = 7;
    result.height = 9;
    Assert(bmp_file.width == 128);
    Assert(bmp_file.height == 64);
    // NOTE: Starts at index 32 cuz thats "space"
    
    result.data = push_array(arena, U32, ASCII_COUNT*result.width*result.height);
    U32 *output_pixels = result.data + (ASCII_CHAR_START*result.width*result.height);
    for (U32 ascii_idx = ASCII_CHAR_START; ascii_idx < ASCII_COUNT; ascii_idx++)
    {
        // NOTE: 18 chars per line, single pixel padding on all sides per char
        //       7x9 per char
        U32 image_idx = ascii_idx - ASCII_CHAR_START;
        U32 row = image_idx / 18;
        U32 col = image_idx % 18;
        U32 *image_pixels = bmp_file.pixels + row*result.height*bmp_file.width + col*result.width;
        for (U32 i=0; i < 9; i++)
        {
            for (U32 j=0; j < 7; j++)
            {
                *output_pixels++ = *(image_pixels+j);
            }
            image_pixels += bmp_file.width;
        }
    }

    return result;
}