#include "fonts.h"
#include "renderer.h"
#include "platform.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

#include <stdlib.h>

void loadGlyph(void)
{
    stbtt_fontinfo fontInfo;
    unsigned char *fontFile = loadFile("/usr/share/fonts/liberation/LiberationSans-Regular.ttf");
    if (fontFile == NULL)
    {
        return;
    }
    if (stbtt_InitFont(&fontInfo, fontFile, 0) == 0)
    {
        return;
    }
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, 100);
    glyphTest.data = stbtt_GetCodepointBitmap(&fontInfo, scale, scale, 'Y', &glyphTest.width, &glyphTest.height, 0, 0);
    free(fontFile);
}
