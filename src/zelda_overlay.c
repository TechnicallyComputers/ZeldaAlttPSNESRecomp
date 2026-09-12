#include "zelda_overlay.h"

#include "snes_osd.h"
#include "snes_savestate_menu.h"
#include "snes_rewind.h"

void ZeldaCompositeOverlay(uint8_t *frame, int pitch, int frame_w, int frame_h,
                           const uint32_t *src, int src_w, int src_h,
                           int half, int dst_x, int dst_y)
{
    int step = half ? 2 : 1;
    int dw = src_w / step, dh = src_h / step;
    int ox = (dst_x == ZELDA_OVERLAY_CENTER) ? (frame_w - dw) / 2 : dst_x;
    int oy = (dst_y == ZELDA_OVERLAY_CENTER) ? (frame_h - dh) / 2 : dst_y;
    int x, y;

    if (!frame || !src || dw <= 0 || dh <= 0)
        return;
    if (ox < 0) ox = 0;
    if (oy < 0) oy = 0;

    for (y = 0; y < dh; y++) {
        int dy = oy + y;
        uint32_t *drow;
        const uint32_t *srow;
        if (dy < 0 || dy >= frame_h)
            continue;
        drow = (uint32_t *)(frame + (size_t)dy * (size_t)pitch);
        srow = src + (size_t)(y * step) * (size_t)src_w;
        for (x = 0; x < dw; x++) {
            int dx = ox + x;
            uint32_t px, d;
            unsigned a, ia, r, g, b;
            if (dx < 0 || dx >= frame_w)
                continue;
            px = srow[(size_t)x * step];
            a = px >> 24;
            if (!a)
                continue;                    /* fully transparent */
            if (a == 0xffu) {
                drow[dx] = px;
                continue;
            }
            d = drow[dx];
            ia = 255u - a;
            r = ((((px >> 16) & 0xffu) * a) + (((d >> 16) & 0xffu) * ia)) / 255u;
            g = ((((px >>  8) & 0xffu) * a) + (((d >>  8) & 0xffu) * ia)) / 255u;
            b = ((( px        & 0xffu) * a) + (( d        & 0xffu) * ia)) / 255u;
            drow[dx] = 0xff000000u | (r << 16) | (g << 8) | b;
        }
    }
}

ZeldaOverlayDrawFn g_zelda_overlay_draw;

void ZeldaDrawHostPanels(void)
{
    const uint32_t *px;
    int w, h;

    if (!g_zelda_overlay_draw)
        return;
    /* Only one panel can be up: the framework refuses to open rewind while
     * the slot browser is showing. */
    if (snes_savestate_menu_overlay_image(&px, &w, &h) && px)
        g_zelda_overlay_draw(px, w, h);
    else if (snes_rewind_overlay_image(&px, &w, &h) && px)
        g_zelda_overlay_draw(px, w, h);
}

void ZeldaCompositeOsd(uint8_t *frame, int pitch, int frame_w, int frame_h)
{
    const uint32_t *px;
    int w, h;

    /* Top-left, halved to frame scale: the OSD is authored at OSD_SCALE 2
     * because it expects to be drawn in window pixels. Halving keeps the
     * glyphs at the 8px the readout this replaced used. */
    if (snes_osd_image(&px, &w, &h) && px) {
        ZeldaCompositeOverlay(frame, pitch, frame_w, frame_h, px, w, h, 1, 2, 2);
        snes_osd_present_done();
    }
}
