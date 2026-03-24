#ifndef _WIN32

#if defined(__has_include)
#  if __has_include(<X11/Xlib.h>) && __has_include(<X11/Xutil.h>) && __has_include(<X11/keysym.h>)
#    define AYM_LINUX_GFX_X11 1
#  else
#    define AYM_LINUX_GFX_X11 0
#  endif
#else
#  define AYM_LINUX_GFX_X11 0
#endif

#if AYM_LINUX_GFX_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#endif

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AYM_GFX_KEY_CAPACITY 2048

typedef struct AymGfxTextCommand {
    char *text;
    long x;
    long y;
    unsigned long pixel;
} AymGfxTextCommand;

#if AYM_LINUX_GFX_X11
static Display *aym_linux_display = NULL;
static int aym_linux_screen = 0;
static Window aym_linux_window = 0;
static GC aym_linux_gc = 0;
static XImage *aym_linux_image = NULL;
static uint8_t *aym_linux_pixels = NULL;
static Visual *aym_linux_visual = NULL;
static unsigned long aym_linux_red_mask = 0;
static unsigned long aym_linux_green_mask = 0;
static unsigned long aym_linux_blue_mask = 0;
static int aym_linux_red_shift = 0;
static int aym_linux_green_shift = 0;
static int aym_linux_blue_shift = 0;
static int aym_linux_red_bits = 8;
static int aym_linux_green_bits = 8;
static int aym_linux_blue_bits = 8;
static long aym_linux_width = 0;
static long aym_linux_height = 0;
static int aym_linux_alive = 0;
static unsigned long aym_linux_current_color = 0;
static Atom aym_linux_wm_delete = 0;
static unsigned char aym_linux_keys[AYM_GFX_KEY_CAPACITY];
static unsigned char aym_linux_key_hits[AYM_GFX_KEY_CAPACITY];
static AymGfxTextCommand *aym_linux_texts = NULL;
static size_t aym_linux_text_count = 0;
static size_t aym_linux_text_capacity = 0;

static int aym_linux_mask_shift(unsigned long mask) {
    int shift = 0;
    if (mask == 0) return 0;
    while ((mask & 1ul) == 0ul) {
        mask >>= 1;
        ++shift;
    }
    return shift;
}

static int aym_linux_mask_bits(unsigned long mask) {
    int bits = 0;
    while (mask != 0ul) {
        bits += (int)(mask & 1ul);
        mask >>= 1;
    }
    return bits > 0 ? bits : 8;
}

static unsigned long aym_linux_scale_channel(long value, int bits) {
    unsigned long maxValue = (bits >= 31) ? 0x7FFFFFFFul : ((1ul << bits) - 1ul);
    long clipped = value;
    if (clipped < 0) clipped = 0;
    if (clipped > 255) clipped = 255;
    return (unsigned long)((clipped * (long)maxValue + 127L) / 255L);
}

static unsigned long aym_linux_make_pixel(long r, long g, long b) {
    unsigned long rr = aym_linux_scale_channel(r, aym_linux_red_bits) << aym_linux_red_shift;
    unsigned long gg = aym_linux_scale_channel(g, aym_linux_green_bits) << aym_linux_green_shift;
    unsigned long bb = aym_linux_scale_channel(b, aym_linux_blue_bits) << aym_linux_blue_shift;
    return (rr & aym_linux_red_mask) |
           (gg & aym_linux_green_mask) |
           (bb & aym_linux_blue_mask);
}

static void aym_linux_reset_keys(void) {
    memset(aym_linux_keys, 0, sizeof(aym_linux_keys));
    memset(aym_linux_key_hits, 0, sizeof(aym_linux_key_hits));
}

static void aym_linux_clear_texts(void) {
    size_t i = 0;
    for (i = 0; i < aym_linux_text_count; ++i) {
        free(aym_linux_texts[i].text);
    }
    aym_linux_text_count = 0;
}

static void aym_linux_release_texts(void) {
    aym_linux_clear_texts();
    free(aym_linux_texts);
    aym_linux_texts = NULL;
    aym_linux_text_capacity = 0;
}

static int aym_linux_store_text(const char *text, long x, long y, unsigned long pixel) {
    char *copy = NULL;
    AymGfxTextCommand *next = NULL;
    if (!text) text = "";
    copy = (char *)malloc(strlen(text) + 1u);
    if (!copy) return 0;
    strcpy(copy, text);
    if (aym_linux_text_count == aym_linux_text_capacity) {
        size_t nextCapacity = (aym_linux_text_capacity == 0) ? 8u : aym_linux_text_capacity * 2u;
        next = (AymGfxTextCommand *)realloc(aym_linux_texts, nextCapacity * sizeof(AymGfxTextCommand));
        if (!next) {
            free(copy);
            return 0;
        }
        aym_linux_texts = next;
        aym_linux_text_capacity = nextCapacity;
    }
    aym_linux_texts[aym_linux_text_count].text = copy;
    aym_linux_texts[aym_linux_text_count].x = x;
    aym_linux_texts[aym_linux_text_count].y = y;
    aym_linux_texts[aym_linux_text_count].pixel = pixel;
    ++aym_linux_text_count;
    return 1;
}

static void aym_linux_set_pixel(long x, long y, unsigned long pixel) {
    unsigned char *dst = NULL;
    if (!aym_linux_image || !aym_linux_pixels) return;
    if (x < 0 || y < 0 || x >= aym_linux_width || y >= aym_linux_height) return;
    dst = aym_linux_pixels +
          (size_t)y * (size_t)aym_linux_image->bytes_per_line +
          (size_t)x * (size_t)((aym_linux_image->bits_per_pixel + 7) / 8);
    switch (aym_linux_image->bits_per_pixel) {
    case 32:
        dst[0] = (unsigned char)(pixel & 0xFFu);
        dst[1] = (unsigned char)((pixel >> 8) & 0xFFu);
        dst[2] = (unsigned char)((pixel >> 16) & 0xFFu);
        dst[3] = (unsigned char)((pixel >> 24) & 0xFFu);
        break;
    case 24:
        dst[0] = (unsigned char)(pixel & 0xFFu);
        dst[1] = (unsigned char)((pixel >> 8) & 0xFFu);
        dst[2] = (unsigned char)((pixel >> 16) & 0xFFu);
        break;
    case 16:
        dst[0] = (unsigned char)(pixel & 0xFFu);
        dst[1] = (unsigned char)((pixel >> 8) & 0xFFu);
        break;
    default:
        XPutPixel(aym_linux_image, x, y, pixel);
        break;
    }
}

static int aym_linux_key_slot(long key) {
    if (key >= 0 && key < AYM_GFX_KEY_CAPACITY) return (int)key;
    return -1;
}

static long aym_linux_translate_key(XKeyEvent *event) {
    KeySym sym = NoSymbol;
    char text[8];
    int written = 0;
    sym = XLookupKeysym(event, 0);
    switch (sym) {
    case XK_Left: return 1003;
    case XK_Right: return 1004;
    case XK_Up: return 1001;
    case XK_Down: return 1002;
    case XK_Escape: return 27;
    case XK_Return:
    case XK_KP_Enter:
        return 13;
    case XK_space:
        return 32;
    case XK_BackSpace:
        return 8;
    case XK_Tab:
        return 9;
    default:
        break;
    }

    written = XLookupString(event, text, (int)sizeof(text), NULL, NULL);
    if (written > 0) {
        unsigned char ch = (unsigned char)text[0];
        if (isalpha(ch)) return (long)tolower(ch);
        if (isdigit(ch)) return (long)ch;
        if (ch >= 1 && ch <= 127) return (long)ch;
    }
    return -1;
}

static void aym_linux_pump_events(void) {
    XEvent event;
    if (!aym_linux_display) {
        aym_linux_alive = 0;
        return;
    }
    while (XPending(aym_linux_display) > 0) {
        XNextEvent(aym_linux_display, &event);
        switch (event.type) {
        case ClientMessage:
            if ((Atom)event.xclient.data.l[0] == aym_linux_wm_delete) {
                aym_linux_alive = 0;
            }
            break;
        case DestroyNotify:
            aym_linux_alive = 0;
            break;
        case FocusOut:
            aym_linux_reset_keys();
            break;
        case KeyPress: {
            long code = aym_linux_translate_key(&event.xkey);
            int slot = aym_linux_key_slot(code);
            if (slot >= 0) {
                aym_linux_keys[slot] = 1;
                aym_linux_key_hits[slot] = 1;
            }
            break;
        }
        case KeyRelease: {
            long code = aym_linux_translate_key(&event.xkey);
            int slot = aym_linux_key_slot(code);
            if (slot >= 0) {
                aym_linux_keys[slot] = 0;
            }
            break;
        }
        default:
            break;
        }
    }
}

static void aym_linux_destroy_image(void) {
    if (aym_linux_image) {
        aym_linux_image->data = (char *)aym_linux_pixels;
        XDestroyImage(aym_linux_image);
        aym_linux_image = NULL;
        aym_linux_pixels = NULL;
    }
}

static void aym_linux_close_window(void) {
    if (aym_linux_display) {
        aym_linux_pump_events();
    }
    aym_linux_destroy_image();
    if (aym_linux_gc) {
        XFreeGC(aym_linux_display, aym_linux_gc);
        aym_linux_gc = 0;
    }
    if (aym_linux_window) {
        XDestroyWindow(aym_linux_display, aym_linux_window);
        aym_linux_window = 0;
    }
    if (aym_linux_display) {
        XCloseDisplay(aym_linux_display);
        aym_linux_display = NULL;
    }
    aym_linux_visual = NULL;
    aym_linux_width = 0;
    aym_linux_height = 0;
    aym_linux_alive = 0;
    aym_linux_current_color = 0;
    aym_linux_reset_keys();
    aym_linux_release_texts();
}
#endif

long aym_linux_gfx_open(long width, long height, const char *title) {
#if AYM_LINUX_GFX_X11
    XSetWindowAttributes attrs;
    unsigned long attrMask = 0;
    if (width <= 0 || height <= 0) return 0;
    if (!getenv("DISPLAY") || !getenv("DISPLAY")[0]) return 0;
    if (aym_linux_display || aym_linux_window) {
        aym_linux_close_window();
    }

    aym_linux_display = XOpenDisplay(NULL);
    if (!aym_linux_display) return 0;

    aym_linux_screen = DefaultScreen(aym_linux_display);
    aym_linux_visual = DefaultVisual(aym_linux_display, aym_linux_screen);
    if (!aym_linux_visual) {
        aym_linux_close_window();
        return 0;
    }

    attrs.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask | FocusChangeMask | StructureNotifyMask;
    attrs.background_pixel = BlackPixel(aym_linux_display, aym_linux_screen);
    attrMask = CWEventMask | CWBackPixel;
    aym_linux_window = XCreateWindow(
        aym_linux_display,
        RootWindow(aym_linux_display, aym_linux_screen),
        0, 0,
        (unsigned int)width,
        (unsigned int)height,
        0,
        DefaultDepth(aym_linux_display, aym_linux_screen),
        InputOutput,
        aym_linux_visual,
        attrMask,
        &attrs);
    if (!aym_linux_window) {
        aym_linux_close_window();
        return 0;
    }

    XStoreName(aym_linux_display, aym_linux_window, (title && title[0]) ? title : "AymaraLang");
    aym_linux_wm_delete = XInternAtom(aym_linux_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(aym_linux_display, aym_linux_window, &aym_linux_wm_delete, 1);
    XMapRaised(aym_linux_display, aym_linux_window);
    XSetInputFocus(aym_linux_display, aym_linux_window, RevertToParent, CurrentTime);

    aym_linux_gc = XCreateGC(aym_linux_display, aym_linux_window, 0, NULL);
    if (!aym_linux_gc) {
        aym_linux_close_window();
        return 0;
    }

    aym_linux_pixels = (uint8_t *)calloc((size_t)width * (size_t)height, 4u);
    if (!aym_linux_pixels) {
        aym_linux_close_window();
        return 0;
    }

    aym_linux_image = XCreateImage(
        aym_linux_display,
        aym_linux_visual,
        (unsigned int)DefaultDepth(aym_linux_display, aym_linux_screen),
        ZPixmap,
        0,
        (char *)aym_linux_pixels,
        (unsigned int)width,
        (unsigned int)height,
        32,
        0);
    if (!aym_linux_image) {
        aym_linux_close_window();
        return 0;
    }

    aym_linux_red_mask = aym_linux_visual->red_mask;
    aym_linux_green_mask = aym_linux_visual->green_mask;
    aym_linux_blue_mask = aym_linux_visual->blue_mask;
    aym_linux_red_shift = aym_linux_mask_shift(aym_linux_red_mask);
    aym_linux_green_shift = aym_linux_mask_shift(aym_linux_green_mask);
    aym_linux_blue_shift = aym_linux_mask_shift(aym_linux_blue_mask);
    aym_linux_red_bits = aym_linux_mask_bits(aym_linux_red_mask);
    aym_linux_green_bits = aym_linux_mask_bits(aym_linux_green_mask);
    aym_linux_blue_bits = aym_linux_mask_bits(aym_linux_blue_mask);
    aym_linux_width = width;
    aym_linux_height = height;
    aym_linux_current_color = aym_linux_make_pixel(255, 255, 255);
    aym_linux_alive = 1;
    aym_linux_reset_keys();
    aym_linux_clear_texts();
    XFlush(aym_linux_display);
    return 1;
#else
    (void)width;
    (void)height;
    (void)title;
    return 0;
#endif
}

long aym_linux_gfx_is_open(void) {
#if AYM_LINUX_GFX_X11
    if (!aym_linux_alive) return 0;
    aym_linux_pump_events();
    return aym_linux_alive ? 1 : 0;
#else
    return 0;
#endif
}

long aym_linux_gfx_clear(long r, long g, long b) {
#if AYM_LINUX_GFX_X11
    unsigned long pixel = 0;
    long x = 0;
    long y = 0;
    if (!aym_linux_alive || !aym_linux_image) return 0;
    aym_linux_pump_events();
    if (!aym_linux_alive) return 0;
    pixel = aym_linux_make_pixel(r, g, b);
    for (y = 0; y < aym_linux_height; ++y) {
        for (x = 0; x < aym_linux_width; ++x) {
            aym_linux_set_pixel(x, y, pixel);
        }
    }
    aym_linux_clear_texts();
    return 1;
#else
    (void)r;
    (void)g;
    (void)b;
    return 0;
#endif
}

long aym_linux_gfx_set_color(long r, long g, long b) {
#if AYM_LINUX_GFX_X11
    aym_linux_current_color = aym_linux_make_pixel(r, g, b);
    return 1;
#else
    (void)r;
    (void)g;
    (void)b;
    return 0;
#endif
}

long aym_linux_gfx_rect4(long x, long y, long w, long h) {
#if AYM_LINUX_GFX_X11
    long xx = 0;
    long yy = 0;
    long x0 = x;
    long y0 = y;
    long x1 = x + w;
    long y1 = y + h;
    if (!aym_linux_alive || !aym_linux_image) return 0;
    aym_linux_pump_events();
    if (!aym_linux_alive) return 0;
    if (w < 0) {
        x0 = x + w;
        x1 = x;
    }
    if (h < 0) {
        y0 = y + h;
        y1 = y;
    }
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > aym_linux_width) x1 = aym_linux_width;
    if (y1 > aym_linux_height) y1 = aym_linux_height;
    for (yy = y0; yy < y1; ++yy) {
        for (xx = x0; xx < x1; ++xx) {
            aym_linux_set_pixel(xx, yy, aym_linux_current_color);
        }
    }
    return 1;
#else
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    return 0;
#endif
}

long aym_linux_gfx_text3(const char *text, long x, long y) {
#if AYM_LINUX_GFX_X11
    if (!aym_linux_alive) return 0;
    aym_linux_pump_events();
    if (!aym_linux_alive) return 0;
    return aym_linux_store_text(text, x, y, aym_linux_current_color);
#else
    (void)text;
    (void)x;
    (void)y;
    return 0;
#endif
}

long aym_linux_gfx_present(void) {
#if AYM_LINUX_GFX_X11
    size_t i = 0;
    if (!aym_linux_alive || !aym_linux_display || !aym_linux_window || !aym_linux_image) return 0;
    aym_linux_pump_events();
    if (!aym_linux_alive) return 0;
    XPutImage(aym_linux_display,
              aym_linux_window,
              aym_linux_gc,
              aym_linux_image,
              0, 0, 0, 0,
              (unsigned int)aym_linux_width,
              (unsigned int)aym_linux_height);
    for (i = 0; i < aym_linux_text_count; ++i) {
        int len = (int)strlen(aym_linux_texts[i].text);
        if (len <= 0) continue;
        XSetForeground(aym_linux_display, aym_linux_gc, aym_linux_texts[i].pixel);
        XDrawString(aym_linux_display,
                    aym_linux_window,
                    aym_linux_gc,
                    (int)aym_linux_texts[i].x,
                    (int)aym_linux_texts[i].y,
                    aym_linux_texts[i].text,
                    len);
    }
    XFlush(aym_linux_display);
    return 1;
#else
    return 0;
#endif
}

long aym_linux_gfx_close(void) {
#if AYM_LINUX_GFX_X11
    aym_linux_close_window();
    return 1;
#else
    return 0;
#endif
}

long aym_linux_gfx_key_down(long key) {
#if AYM_LINUX_GFX_X11
    int slot = 0;
    if (!aym_linux_alive) return 0;
    aym_linux_pump_events();
    if (!aym_linux_alive) return 0;
    slot = aym_linux_key_slot(key);
    if (slot < 0) return 0;
    if (aym_linux_key_hits[slot]) {
        aym_linux_key_hits[slot] = 0;
        return 1;
    }
    return aym_linux_keys[slot] ? 1 : 0;
#else
    (void)key;
    return 0;
#endif
}

#endif
