#include "fonts.h"
#include "renderer.h"
#include "platform.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

#include <stdlib.h>

void loadFont(void)
{
    stbtt_fontinfo fontInfo;
    void *fontFile = loadFile("assets/fonts/LiberationSans-Regular.ttf");
    if (fontFile == NULL)
    {
        return;
    }
    if (stbtt_InitFont(&fontInfo, fontFile, 0) == 0)
    {
        debugLog("stb_truetype failed to initilize");
        return;
    }
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, 35);
    for (int i = 0; i < 94; i++)
    {
        int glyphIndex = stbtt_FindGlyphIndex(&fontInfo, i + 33);
        glyphs[i].data = stbtt_GetGlyphBitmap(&fontInfo, scale, scale, glyphIndex, &glyphs[i].width, &glyphs[i].height, &glyphs[i].xOffset, &glyphs[i].yOffset);
        int advance;
        stbtt_GetGlyphHMetrics(&fontInfo, glyphIndex, &advance, NULL);
        glyphs[i].advance = advance * scale;
    }
    free(fontFile);
}
