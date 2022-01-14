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
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, 50);
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    fontMetrics.ascent = ascent * scale;
    fontMetrics.descent = descent * scale;
    fontMetrics.lineGap = lineGap * scale;
    int glyphIndices[94];
    for (int i = 0; i < 94; i++)
    {
        glyphIndices[i] = stbtt_FindGlyphIndex(&fontInfo, i + 33);
    }
    for (int i = 0; i < 94; i++)
    {
        glyphs[i].data = stbtt_GetGlyphBitmap(&fontInfo, scale, scale, glyphIndices[i], &glyphs[i].width, &glyphs[i].height, &glyphs[i].xOffset, &glyphs[i].yOffset);
        int advance;
        stbtt_GetGlyphHMetrics(&fontInfo, glyphIndices[i], &advance, NULL);
        glyphs[i].advance = advance * scale;
        for (int j = 0; j < 94; j++)
        {
            glyphs[i].kerning[j] = stbtt_GetGlyphKernAdvance(&fontInfo, glyphIndices[i], glyphIndices[j]) * scale;
        }
    }
    free(fontFile);
}
