#ifndef ZELDA_OVERLAY_H
#define ZELDA_OVERLAY_H

#include <stddef.h>
#include <stdint.h>

/*
 * Compositing the framework's host overlays onto the present buffer.
 *
 * Its own unit rather than a static in main.c for two reasons. This host
 * presents through g_renderer_funcs -- an SDL texture on one backend, OpenGL
 * on the other -- so snes_osd_draw_sdl(), which only serves the SDL_Renderer
 * path, would give the two backends different overlays. And a pixel routine
 * with no SDL, no PPU and no globals in it can be run against the framework's
 * real rasterizers on its own, which is the only way to see what it draws
 * without a display attached.
 */

/* Alpha-blend an ARGB8888 overlay onto an XRGB8888 frame.
 *
 * `half` point-samples the source 2:1. Everything the framework rasterizes is
 * authored at 2x the SNES frame -- the panels at SNES_SSM_W x SNES_SSM_H so a
 * host overlaying the game TEXTURE needs no aspect correction, the OSD at
 * OSD_SCALE 2 because it is meant to be drawn in window pixels. Both are
 * halved here, which is what puts them back at frame scale.
 *
 * `x` and `y` are the destination origin after halving; pass
 * ZELDA_OVERLAY_CENTER for either axis to center on it.
 *
 * Clips on all four sides, so a panel wider than the frame (or a frame
 * widened by widescreen) is safe.
 */
#define ZELDA_OVERLAY_CENTER (-32768)

void ZeldaCompositeOverlay(uint8_t *frame, int pitch, int frame_w, int frame_h,
                           const uint32_t *src, int src_w, int src_h,
                           int half, int x, int y);

/* The OSD status line and toasts, composited into the frame at top-left.
 *
 * Deliberately NOT snes_osd_draw_sdl(), which draws at {8,8} in window pixels
 * because host chrome should not stretch with the aspect-corrected frame.
 * Neither backend here can take that path: the OpenGL one has no SDL_Renderer
 * at all, and the SDL one gives its renderer a 256x224 logical size, which
 * would clip the OSD's 1032-pixel canvas to the left third of the screen. */
void ZeldaCompositeOsd(uint8_t *frame, int pitch, int frame_w, int frame_h);

/* ---- modal panels ------------------------------------------------------ */
/*
 * The save-state browser and the rewind filmstrip are drawn as a TEXTURE over
 * the presented frame, at the same destination rect as the game -- which is
 * what they are authored for, at 2x the SNES frame. Compositing them into the
 * 256-wide frame instead meant halving them first, and a panel at half its
 * authored resolution does not look like the one every other port shows.
 *
 * Each renderer backend installs the hook it can honour; nothing else about
 * the panels differs between them.
 */
typedef void (*ZeldaOverlayDrawFn)(const uint32_t *pixels, int w, int h);
extern ZeldaOverlayDrawFn g_zelda_overlay_draw;

/* Called by a backend's EndDraw, after the game frame and before the present.
 * No-ops when no panel is open or no hook is installed. */
void ZeldaDrawHostPanels(void);

#endif /* ZELDA_OVERLAY_H */
