// simply a wrapper for stb_image_write.h due to the many compiler warnings

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int wrap_stbi_write_png(char const *filename, int w, int h, int comp, const void *data, int stride_in_bytes)
{
    return stbi_write_png(filename, w, h, comp, data, stride_in_bytes);
}