#ifndef PPT_SCALE_H
#define PPT_SCALE_H

#include <stdint.h>
#include <stddef.h>

typedef struct { int x, y, w, h; } PPTScaleRect;

/* Compose once into a real output-sized ARGB8888 frame.
 * This is deliberately NOT an SDL_RenderCopy into another scaled viewport.
 * The compositor sees a single 1:1 640x480 texture. */
static inline int ppt_compose_frame(const uint32_t *src, int srcW, int srcH,
                                    uint32_t *dst, int outW, int outH,
                                    PPTScaleRect *result)
{
    if (!src || !dst || srcW <= 0 || srcH <= 0 || outW <= 0 || outH <= 0)
        return 0;
    int drawW, drawH;
    if ((int64_t)outW * srcH >= (int64_t)outH * srcW) {
        drawH = outH;
        drawW = (int)((int64_t)outH * srcW / srcH);
    } else {
        drawW = outW;
        drawH = (int)((int64_t)outW * srcH / srcW);
    }
    const int offX = (outW - drawW) / 2;
    const int offY = (outH - drawH) / 2;
    if (result) *result = (PPTScaleRect){offX, offY, drawW, drawH};
    for (int y = 0; y < outH; ++y) {
        uint32_t *line = dst + (size_t)y * outW;
        for (int x = 0; x < outW; ++x) line[x] = 0xFF000000u;
        if (y < offY || y >= offY + drawH) continue;
        int sy = (int)((int64_t)(2 * (y - offY) + 1) * srcH / (2 * drawH));
        if (sy >= srcH) sy = srcH - 1;
        const uint32_t *source = src + (size_t)sy * srcW;
        for (int x = 0; x < drawW; ++x) {
            int sx = (int)((int64_t)(2 * x + 1) * srcW / (2 * drawW));
            if (sx >= srcW) sx = srcW - 1;
            line[offX + x] = source[sx];
        }
    }
    return 1;
}
#endif
