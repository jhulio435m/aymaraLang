#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <setjmp.h>
#include <ctype.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <termios.h>
#include <fcntl.h>
#endif

intptr_t aym_exception_new(const char *type, const char *message);
void aym_throw(intptr_t exception);
intptr_t aym_array_new(long size);
intptr_t aym_array_get(intptr_t arr, long idx);
long aym_array_length(intptr_t arr);
intptr_t aym_array_push(intptr_t arr, intptr_t val);
long aym_gfx_set_color(long r, long g, long b);
long aym_gfx_rect4(long x, long y, long w, long h);
long aym_gfx_text3(const char *text, long x, long y);

static void aym_throw_typed(const char *type, const char *message) {
    intptr_t exc = aym_exception_new(type, message);
    aym_throw(exc);
}

char *aym_str_concat(const char *left, const char *right);

static char *aym_str_copy(const char *src, size_t len) {
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    if (len && src) {
        memcpy(out, src, len);
    }
    out[len] = '\0';
    return out;
}

static int aym_is_word_char(char ch) {
    return isalnum((unsigned char)ch) || ch == '_';
}

void leer_linea(char *buf, int size) {
    if (fgets(buf, size, stdin)) {
        size_t len = strlen(buf);
        if (len && buf[len-1] == '\n') buf[len-1] = '\0';
    } else {
        if (size) buf[0] = '\0';
    }
}

static int seeded = 0;

void aym_srand(unsigned int seed) {
    srand(seed);
    seeded = 1;
}

int aym_random(int max) {
    if (!seeded) {
        aym_srand((unsigned)time(NULL));
    }
    if (max > 0) return rand() % max;
    return rand();
}

void aym_sleep(int ms) {
    if (ms <= 0) return;
#ifdef _WIN32
    Sleep(ms);
#else
    usleep((useconds_t)ms * 1000);
#endif
}

static long aym_cli_argc = 0;
static const char **aym_cli_argv = NULL;

void aym_set_args(long argc, const char **argv) {
    aym_cli_argc = argc;
    aym_cli_argv = argv;
}

long aym_argc(void) {
    return aym_cli_argc;
}

const char *aym_argv_get(long idx) {
    if (!aym_cli_argv) return "";
    if (idx < 0 || idx >= aym_cli_argc) return "";
    if (!aym_cli_argv[idx]) return "";
    return aym_cli_argv[idx];
}

void aym_exit(long code) {
    exit((int)code);
}

long aym_assert(long cond, const char *message) {
    if (!cond) {
        aym_throw_typed("ASSERT", message ? message : "afirma fallo");
        return 0;
    }
    return 1;
}

typedef intptr_t (*AymUnaryFn)(intptr_t);
typedef intptr_t (*AymBinaryFn)(intptr_t, intptr_t);

intptr_t aym_hof_map(intptr_t arr, intptr_t fn_ptr) {
    if (!arr || !fn_ptr) return aym_array_new(0);
    long len = aym_array_length(arr);
    intptr_t out = aym_array_new(0);
    AymUnaryFn fn = (AymUnaryFn)fn_ptr;
    for (long i = 0; i < len; i++) {
        intptr_t v = aym_array_get(arr, i);
        intptr_t mapped = fn(v);
        aym_array_push(out, mapped);
    }
    return out;
}

intptr_t aym_hof_filter(intptr_t arr, intptr_t fn_ptr) {
    if (!arr || !fn_ptr) return aym_array_new(0);
    long len = aym_array_length(arr);
    intptr_t out = aym_array_new(0);
    AymUnaryFn fn = (AymUnaryFn)fn_ptr;
    for (long i = 0; i < len; i++) {
        intptr_t v = aym_array_get(arr, i);
        if (fn(v)) {
            aym_array_push(out, v);
        }
    }
    return out;
}

intptr_t aym_hof_reduce(intptr_t arr, intptr_t fn_ptr, intptr_t init) {
    if (!arr || !fn_ptr) return init;
    long len = aym_array_length(arr);
    intptr_t acc = init;
    AymBinaryFn fn = (AymBinaryFn)fn_ptr;
    for (long i = 0; i < len; i++) {
        intptr_t v = aym_array_get(arr, i);
        acc = fn(acc, v);
    }
    return acc;
}

char *aym_fs_read_text(const char *path) {
    if (!path || !path[0]) return aym_str_copy("", 0);
    FILE *f = fopen(path, "rb");
    if (!f) return aym_str_copy("", 0);
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return aym_str_copy("", 0);
    }
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return aym_str_copy("", 0);
    }
    rewind(f);
    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return aym_str_copy("", 0);
    }
    size_t n = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

long aym_fs_write_text(const char *path, const char *text) {
    if (!path || !path[0]) return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    if (!text) text = "";
    size_t len = strlen(text);
    size_t written = fwrite(text, 1, len, f);
    fclose(f);
    return (written == len) ? 1 : 0;
}

long aym_fs_exists(const char *path) {
    if (!path || !path[0]) return 0;
    struct stat st;
    return stat(path, &st) == 0 ? 1 : 0;
}

static int aym_term_initialized = 0;
#ifndef _WIN32
static struct termios aym_orig_termios;
static int aym_termios_active = 0;
static void aym_term_restore(void);
#endif

static void aym_term_init(void) {
    if (aym_term_initialized) return;
    aym_term_initialized = 1;
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &mode)) {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
    }
#else
    if (isatty(STDIN_FILENO)) {
        struct termios raw;
        if (tcgetattr(STDIN_FILENO, &aym_orig_termios) == 0) {
            raw = aym_orig_termios;
            raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
                aym_termios_active = 1;
                atexit(aym_term_restore);
            }
        }
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        if (flags >= 0) {
            (void)fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        }
    }
#endif
}

#ifndef _WIN32
static void aym_term_restore(void) {
    if (aym_termios_active) {
        (void)tcsetattr(STDIN_FILENO, TCSANOW, &aym_orig_termios);
        aym_termios_active = 0;
    }
}
#endif

void aym_term_clear(void) {
    aym_term_init();
    printf("\x1b[2J\x1b[H");
    fflush(stdout);
}

void aym_term_move(long x, long y) {
    aym_term_init();
    if (x < 1) x = 1;
    if (y < 1) y = 1;
    printf("\x1b[%ld;%ldH", y, x);
    fflush(stdout);
}

void aym_term_color(long fg, long bg) {
    aym_term_init();
    if (fg < 0) fg = 7;
    if (bg < 0) bg = 0;
    if (fg > 15) fg = 15;
    if (bg > 15) bg = 15;
    long fgCode = (fg < 8) ? (30 + fg) : (90 + (fg - 8));
    long bgCode = (bg < 8) ? (40 + bg) : (100 + (bg - 8));
    printf("\x1b[%ld;%ldm", fgCode, bgCode);
    fflush(stdout);
}

void aym_term_reset(void) {
    aym_term_init();
    printf("\x1b[0m");
    fflush(stdout);
}

void aym_term_cursor(long visible) {
    aym_term_init();
    printf(visible ? "\x1b[?25h" : "\x1b[?25l");
    fflush(stdout);
}

#ifdef _WIN32
static int aym_key_down(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

static long aym_key_ascii(void) {
    for (int vk = 'A'; vk <= 'Z'; ++vk) {
        if (aym_key_down(vk)) return (long)(vk + ('a' - 'A'));
    }
    for (int vk = '0'; vk <= '9'; ++vk) {
        if (aym_key_down(vk)) return (long)vk;
    }
    if (aym_key_down(VK_SPACE)) return (long)' ';
    if (aym_key_down(VK_RETURN)) return (long)'\r';
    if (aym_key_down(VK_ESCAPE)) return 27;
    if (aym_key_down(VK_BACK)) return 8;
    if (aym_key_down(VK_TAB)) return (long)'\t';
    return -1;
}
#endif

long aym_key_poll(void) {
    aym_term_init();
#ifdef _WIN32
    if (aym_key_down(VK_LEFT)) return 1003;
    if (aym_key_down(VK_RIGHT)) return 1004;
    if (aym_key_down(VK_DOWN)) return 1002;
    if (aym_key_down(VK_UP)) return 1001;
    long ascii = aym_key_ascii();
    if (ascii != -1) return ascii;
    return -1;
#else
    unsigned char ch = 0;
    ssize_t n = read(STDIN_FILENO, &ch, 1);
    if (n <= 0) return -1;
    if (ch == 27) {
        unsigned char seq[2];
        ssize_t n1 = read(STDIN_FILENO, &seq[0], 1);
        ssize_t n2 = read(STDIN_FILENO, &seq[1], 1);
        if (n1 == 1 && n2 == 1 && seq[0] == '[') {
            if (seq[1] == 'A') return 1001; // up
            if (seq[1] == 'B') return 1002; // down
            if (seq[1] == 'D') return 1003; // left
            if (seq[1] == 'C') return 1004; // right
        }
        return 27;
    }
    return (long)ch;
#endif
}

long aym_time_ms(void) {
#ifdef _WIN32
    return (long)GetTickCount64();
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) return 0;
    return (long)(tv.tv_sec * 1000L + tv.tv_usec / 1000L);
#endif
}

static long aym_color_clip(long value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return value;
}

#ifdef _WIN32
static HWND aym_gfx_hwnd = NULL;
static HDC aym_gfx_window_dc = NULL;
static HDC aym_gfx_mem_dc = NULL;
static HBITMAP aym_gfx_bitmap = NULL;
static HBITMAP aym_gfx_old_bitmap = NULL;
static HFONT aym_gfx_font = NULL;
static uint32_t *aym_gfx_pixels = NULL;
static unsigned char aym_gfx_keys[256];
static unsigned char aym_gfx_key_hits[256];
static HWND aym_gfx_console_hwnd = NULL;
static int aym_gfx_console_hidden = 0;
static int aym_gfx_console_was_visible = 0;
static uint32_t aym_gfx_current_color = 0x00FFFFFFu;
static BITMAPINFO aym_gfx_bmi;
static long aym_gfx_width = 0;
static long aym_gfx_height = 0;
static int aym_gfx_alive = 0;
static int aym_gfx_class_ready = 0;
static const wchar_t *aym_gfx_class_name = L"AymaraLangGfxWindow";

static void aym_gfx_pump_messages(void) {
    MSG msg;
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (!aym_gfx_hwnd || !IsWindow(aym_gfx_hwnd)) {
        aym_gfx_alive = 0;
    }
}

static wchar_t *aym_gfx_utf8_to_wide(const char *text) {
    if (!text || !text[0]) text = "AymaraLang";
    int needed = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (needed <= 0) {
        needed = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
        if (needed <= 0) return NULL;
        wchar_t *out = (wchar_t *)calloc((size_t)needed, sizeof(wchar_t));
        if (!out) return NULL;
        if (MultiByteToWideChar(CP_ACP, 0, text, -1, out, needed) <= 0) {
            free(out);
            return NULL;
        }
        return out;
    }
    wchar_t *out = (wchar_t *)calloc((size_t)needed, sizeof(wchar_t));
    if (!out) return NULL;
    if (MultiByteToWideChar(CP_UTF8, 0, text, -1, out, needed) <= 0) {
        free(out);
        return NULL;
    }
    return out;
}

static LRESULT CALLBACK aym_gfx_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    (void)lparam;
    switch (msg) {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        unsigned int vk = (unsigned int)(wparam & 0xFFu);
        aym_gfx_keys[vk] = 1;
        aym_gfx_key_hits[vk] = 1;
        if (vk == VK_ESCAPE) {
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;
    }
    case WM_CHAR:
        if (wparam == 'q' || wparam == 'Q') {
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP: {
        unsigned int vk = (unsigned int)(wparam & 0xFFu);
        aym_gfx_keys[vk] = 0;
        return 0;
    }
    case WM_KILLFOCUS:
        ZeroMemory(aym_gfx_keys, sizeof(aym_gfx_keys));
        ZeroMemory(aym_gfx_key_hits, sizeof(aym_gfx_key_hits));
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        if (hwnd == aym_gfx_hwnd) {
            aym_gfx_alive = 0;
            aym_gfx_hwnd = NULL;
        }
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

static void aym_gfx_release_buffers(void) {
    if (aym_gfx_mem_dc) {
        if (aym_gfx_old_bitmap) {
            SelectObject(aym_gfx_mem_dc, aym_gfx_old_bitmap);
        }
        aym_gfx_old_bitmap = NULL;
        DeleteDC(aym_gfx_mem_dc);
        aym_gfx_mem_dc = NULL;
    }
    if (aym_gfx_bitmap) {
        DeleteObject(aym_gfx_bitmap);
        aym_gfx_bitmap = NULL;
    }
    if (aym_gfx_font) {
        DeleteObject(aym_gfx_font);
        aym_gfx_font = NULL;
    }
    aym_gfx_pixels = NULL;
    aym_gfx_width = 0;
    aym_gfx_height = 0;
}

static void aym_gfx_close_window(void) {
    HWND hwnd = aym_gfx_hwnd;
    if (aym_gfx_window_dc && hwnd) {
        ReleaseDC(hwnd, aym_gfx_window_dc);
    }
    aym_gfx_window_dc = NULL;
    ZeroMemory(aym_gfx_keys, sizeof(aym_gfx_keys));
    ZeroMemory(aym_gfx_key_hits, sizeof(aym_gfx_key_hits));

    aym_gfx_release_buffers();

    if (hwnd && IsWindow(hwnd)) {
        DestroyWindow(hwnd);
    }
    if (aym_gfx_console_hidden && aym_gfx_console_hwnd &&
        IsWindow(aym_gfx_console_hwnd) && aym_gfx_console_was_visible) {
        ShowWindow(aym_gfx_console_hwnd, SW_SHOW);
    }
    aym_gfx_console_hwnd = NULL;
    aym_gfx_console_hidden = 0;
    aym_gfx_console_was_visible = 0;
    aym_gfx_hwnd = NULL;
    aym_gfx_alive = 0;
}

static int aym_gfx_key_to_vk(long key) {
    switch (key) {
    case 1001: return VK_UP;
    case 1002: return VK_DOWN;
    case 1003: return VK_LEFT;
    case 1004: return VK_RIGHT;
    case 27: return VK_ESCAPE;
    case 13: return VK_RETURN;
    case 32: return VK_SPACE;
    case 8: return VK_BACK;
    case 9: return VK_TAB;
    default: break;
    }
    if (key >= 'a' && key <= 'z') return (int)(key - ('a' - 'A'));
    if (key >= 'A' && key <= 'Z') return (int)key;
    if (key >= '0' && key <= '9') return (int)key;
    if (key >= 1 && key <= 255) return (int)key;
    return 0;
}
#endif

long aym_gfx_open(long width, long height, const char *title) {
#ifdef _WIN32
    if (width <= 0 || height <= 0) return 0;
    if (width > 4096 || height > 4096) return 0;

    if (aym_gfx_alive || aym_gfx_hwnd) {
        aym_gfx_close_window();
    }

    HINSTANCE instance = GetModuleHandleW(NULL);
    if (!aym_gfx_class_ready) {
        WNDCLASSEXW wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = aym_gfx_wndproc;
        wc.hInstance = instance;
        wc.lpszClassName = aym_gfx_class_name;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return 0;
        }
        aym_gfx_class_ready = 1;
    }

    RECT rect = {0, 0, (LONG)width, (LONG)height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    wchar_t *wideTitle = aym_gfx_utf8_to_wide(title);
    HWND hwnd = CreateWindowExW(
        0,
        aym_gfx_class_name,
        wideTitle ? wideTitle : L"AymaraLang",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL, NULL, instance, NULL);
    free(wideTitle);
    if (!hwnd) {
        return 0;
    }

    HDC windowDC = GetDC(hwnd);
    if (!windowDC) {
        DestroyWindow(hwnd);
        return 0;
    }

    HDC memDC = CreateCompatibleDC(windowDC);
    if (!memDC) {
        ReleaseDC(hwnd, windowDC);
        DestroyWindow(hwnd);
        return 0;
    }

    ZeroMemory(&aym_gfx_bmi, sizeof(aym_gfx_bmi));
    aym_gfx_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    aym_gfx_bmi.bmiHeader.biWidth = (LONG)width;
    aym_gfx_bmi.bmiHeader.biHeight = -(LONG)height;
    aym_gfx_bmi.bmiHeader.biPlanes = 1;
    aym_gfx_bmi.bmiHeader.biBitCount = 32;
    aym_gfx_bmi.bmiHeader.biCompression = BI_RGB;

    void *pixels = NULL;
    HBITMAP bitmap = CreateDIBSection(windowDC, &aym_gfx_bmi, DIB_RGB_COLORS, &pixels, NULL, 0);
    if (!bitmap || !pixels) {
        DeleteDC(memDC);
        ReleaseDC(hwnd, windowDC);
        DestroyWindow(hwnd);
        return 0;
    }

    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, bitmap);
    HFONT font = CreateFontA(
        -18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, "Consolas");
    if (font) {
        SelectObject(memDC, font);
    }
    SetBkMode(memDC, TRANSPARENT);

    aym_gfx_hwnd = hwnd;
    aym_gfx_window_dc = windowDC;
    aym_gfx_mem_dc = memDC;
    aym_gfx_bitmap = bitmap;
    aym_gfx_old_bitmap = oldBitmap;
    aym_gfx_font = font;
    aym_gfx_pixels = (uint32_t *)pixels;
    ZeroMemory(aym_gfx_keys, sizeof(aym_gfx_keys));
    ZeroMemory(aym_gfx_key_hits, sizeof(aym_gfx_key_hits));
    aym_gfx_width = width;
    aym_gfx_height = height;
    aym_gfx_alive = 1;

    aym_gfx_console_hwnd = GetConsoleWindow();
    aym_gfx_console_was_visible = (aym_gfx_console_hwnd && IsWindowVisible(aym_gfx_console_hwnd)) ? 1 : 0;
    if (aym_gfx_console_was_visible) {
        ShowWindow(aym_gfx_console_hwnd, SW_HIDE);
        aym_gfx_console_hidden = 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    aym_gfx_pump_messages();
    return 1;
#else
    (void)width;
    (void)height;
    (void)title;
    return 0;
#endif
}

long aym_gfx_is_open(void) {
#ifdef _WIN32
    aym_gfx_pump_messages();
    return aym_gfx_alive ? 1 : 0;
#else
    return 0;
#endif
}

long aym_gfx_clear(long r, long g, long b) {
#ifdef _WIN32
    if (!aym_gfx_alive || !aym_gfx_pixels) return 0;
    aym_gfx_pump_messages();
    if (!aym_gfx_alive) return 0;

    uint32_t color = ((uint32_t)aym_color_clip(r) << 16) |
                     ((uint32_t)aym_color_clip(g) << 8) |
                     (uint32_t)aym_color_clip(b);
    size_t total = (size_t)aym_gfx_width * (size_t)aym_gfx_height;
    for (size_t i = 0; i < total; ++i) {
        aym_gfx_pixels[i] = color;
    }
    return 1;
#else
    (void)r;
    (void)g;
    (void)b;
    return 0;
#endif
}

long aym_gfx_rect(long x, long y, long w, long h, long r, long g, long b) {
#ifdef _WIN32
    aym_gfx_current_color = ((uint32_t)aym_color_clip(r) << 16) |
                            ((uint32_t)aym_color_clip(g) << 8) |
                            (uint32_t)aym_color_clip(b);
    return aym_gfx_rect4(x, y, w, h);
#else
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)r;
    (void)g;
    (void)b;
    return 0;
#endif
}

long aym_gfx_set_color(long r, long g, long b) {
#ifdef _WIN32
    aym_gfx_current_color = ((uint32_t)aym_color_clip(r) << 16) |
                            ((uint32_t)aym_color_clip(g) << 8) |
                            (uint32_t)aym_color_clip(b);
    return 1;
#else
    (void)r;
    (void)g;
    (void)b;
    return 0;
#endif
}

long aym_gfx_rect4(long x, long y, long w, long h) {
#ifdef _WIN32
    if (!aym_gfx_alive || !aym_gfx_pixels) return 0;
    aym_gfx_pump_messages();
    if (!aym_gfx_alive) return 0;

    if (w < 0) {
        x += w;
        w = -w;
    }
    if (h < 0) {
        y += h;
        h = -h;
    }
    if (w <= 0 || h <= 0) return 1;

    long x0 = x < 0 ? 0 : x;
    long y0 = y < 0 ? 0 : y;
    long x1 = x + w;
    long y1 = y + h;
    if (x1 > aym_gfx_width) x1 = aym_gfx_width;
    if (y1 > aym_gfx_height) y1 = aym_gfx_height;
    if (x0 >= x1 || y0 >= y1) return 1;

    for (long yy = y0; yy < y1; ++yy) {
        size_t row = (size_t)yy * (size_t)aym_gfx_width;
        for (long xx = x0; xx < x1; ++xx) {
            aym_gfx_pixels[row + (size_t)xx] = aym_gfx_current_color;
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

long aym_gfx_text(const char *text, long x, long y, long r, long g, long b) {
#ifdef _WIN32
    aym_gfx_current_color = ((uint32_t)aym_color_clip(r) << 16) |
                            ((uint32_t)aym_color_clip(g) << 8) |
                            (uint32_t)aym_color_clip(b);
    return aym_gfx_text3(text, x, y);
#else
    (void)text;
    (void)x;
    (void)y;
    (void)r;
    (void)g;
    (void)b;
    return 0;
#endif
}

long aym_gfx_text3(const char *text, long x, long y) {
#ifdef _WIN32
    if (!aym_gfx_alive || !aym_gfx_mem_dc) return 0;
    aym_gfx_pump_messages();
    if (!aym_gfx_alive) return 0;
    if (!text) text = "";

    SetTextColor(aym_gfx_mem_dc, RGB(
        (int)((aym_gfx_current_color >> 16) & 0xFFu),
        (int)((aym_gfx_current_color >> 8) & 0xFFu),
        (int)(aym_gfx_current_color & 0xFFu)));
    SetBkMode(aym_gfx_mem_dc, TRANSPARENT);
    int len = (int)strlen(text);
    if (len <= 0) return 1;
    return TextOutA(aym_gfx_mem_dc, (int)x, (int)y, text, len) ? 1 : 0;
#else
    (void)text;
    (void)x;
    (void)y;
    return 0;
#endif
}

long aym_gfx_present(void) {
#ifdef _WIN32
    if (!aym_gfx_alive || !aym_gfx_window_dc || !aym_gfx_mem_dc) return 0;
    aym_gfx_pump_messages();
    if (!aym_gfx_alive) return 0;
    return BitBlt(
        aym_gfx_window_dc, 0, 0, (int)aym_gfx_width, (int)aym_gfx_height,
        aym_gfx_mem_dc, 0, 0, SRCCOPY) ? 1 : 0;
#else
    return 0;
#endif
}

long aym_gfx_close(void) {
#ifdef _WIN32
    aym_gfx_close_window();
    return 1;
#else
    return 0;
#endif
}

long aym_gfx_key_down(long key) {
#ifdef _WIN32
    if (!aym_gfx_alive) return 0;
    aym_gfx_pump_messages();
    if (!aym_gfx_alive) return 0;
    int vk = aym_gfx_key_to_vk(key);
    if (vk == 0) return 0;
    if (vk >= 0 && vk < 256) {
        if (aym_gfx_key_hits[vk]) {
            aym_gfx_key_hits[vk] = 0;
            return 1;
        }
        if (aym_gfx_keys[vk]) return 1;
    }
    return (GetAsyncKeyState(vk) & 0x8000) ? 1 : 0;
#else
    (void)key;
    return 0;
#endif
}

char *aym_file_read_text(const char *path) {
    if (!path || path[0] == '\0') return aym_str_copy("", 0);
    FILE *f = fopen(path, "rb");
    if (!f) return aym_str_copy("", 0);
    size_t cap = 256;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) {
        fclose(f);
        return aym_str_copy("", 0);
    }
    int ch = 0;
    while ((ch = fgetc(f)) != EOF) {
        if (len + 1 >= cap) {
            size_t nextCap = cap * 2;
            char *next = (char *)realloc(buf, nextCap);
            if (!next) {
                free(buf);
                fclose(f);
                return aym_str_copy("", 0);
            }
            buf = next;
            cap = nextCap;
        }
        buf[len++] = (char)ch;
    }
    fclose(f);
    if (len + 1 >= cap) {
        char *next = (char *)realloc(buf, len + 1);
        if (!next) {
            free(buf);
            return aym_str_copy("", 0);
        }
        buf = next;
    }
    buf[len] = '\0';
    return buf;
}

long aym_file_write_text(const char *path, const char *text) {
    if (!path || path[0] == '\0') return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    if (!text) text = "";
    size_t len = strlen(text);
    size_t written = fwrite(text, 1, len, f);
    fclose(f);
    return (written == len) ? 1 : 0;
}

long aym_record_load(void) {
    FILE *f = fopen("tetris_highscore.txt", "rb");
    if (!f) return 0;
    long value = 0;
    if (fscanf(f, "%ld", &value) != 1) value = 0;
    fclose(f);
    return value;
}

long aym_record_save(long value) {
    FILE *f = fopen("tetris_highscore.txt", "wb");
    if (!f) return 0;
    int ok = fprintf(f, "%ld", value);
    fclose(f);
    return ok > 0 ? 1 : 0;
}

typedef struct {
    long len;
    long cap;
    intptr_t *data;
} AymArray;

intptr_t aym_array_new(long size) {
    if (size < 0) return 0;
    AymArray *arr = calloc(1, sizeof(AymArray));
    if (!arr) {
        fprintf(stderr, "aym_array_new: allocation failed\n");
        return 0;
    }
    arr->len = size;
    arr->cap = size;
    if (size > 0) {
        arr->data = calloc((size_t)size, sizeof(intptr_t));
        if (!arr->data) {
            fprintf(stderr, "aym_array_new: allocation failed\n");
            free(arr);
            return 0;
        }
    }
    return (intptr_t)arr;
}

intptr_t aym_array_get(intptr_t arr, long idx) {
    if (!arr) return 0;
    AymArray *a = (AymArray*)arr;
    if (idx < 0) {
        fprintf(stderr, "aym_array_get: negative index %ld\n", idx);
        return 0; // default value on error
    }
    if (idx >= a->len) return 0;
    return a->data[idx];
}

intptr_t aym_array_set(intptr_t arr, long idx, intptr_t val) {
    if (!arr) return 0;
    AymArray *a = (AymArray*)arr;
    if (idx < 0) {
        fprintf(stderr, "aym_array_set: negative index %ld\n", idx);
        return 0; // indicate error
    }
    if (idx >= a->len) return 0;
    a->data[idx] = (intptr_t)val;
    return val;
}

void aym_array_free(intptr_t arr) {
    if (!arr) return;
    AymArray *a = (AymArray*)arr;
    free(a->data);
    free(a);
}

long aym_array_length(intptr_t arr) {
    if (!arr) return 0;
    AymArray *a = (AymArray*)arr;
    return a->len;
}

intptr_t aym_array_push(intptr_t arr, intptr_t val) {
    if (!arr) return 0;
    AymArray *a = (AymArray*)arr;
    if (a->len >= a->cap) {
        long newCap = a->cap > 0 ? a->cap * 2 : 1;
        intptr_t *next = realloc(a->data, sizeof(intptr_t) * (size_t)newCap);
        if (!next) {
            fprintf(stderr, "aym_array_push: allocation failed\n");
            return arr;
        }
        a->data = next;
        a->cap = newCap;
    }
    a->data[a->len++] = val;
    return arr;
}

intptr_t aym_array_pop(intptr_t arr) {
    if (!arr) {
        aym_throw_typed("VACIO", "lista vacia");
        return 0;
    }
    AymArray *a = (AymArray*)arr;
    if (a->len <= 0) {
        aym_throw_typed("VACIO", "lista vacia");
        return 0;
    }
    intptr_t value = a->data[a->len - 1];
    a->len--;
    return value;
}

intptr_t aym_array_remove_at(intptr_t arr, long idx) {
    if (!arr) {
        aym_throw_typed("INDICE", "fuera de rango");
        return 0;
    }
    AymArray *a = (AymArray*)arr;
    if (idx < 0 || idx >= a->len) {
        aym_throw_typed("INDICE", "fuera de rango");
        return 0;
    }
    intptr_t value = a->data[idx];
    for (long i = idx + 1; i < a->len; i++) {
        a->data[i - 1] = a->data[i];
    }
    a->len--;
    return value;
}

long aym_array_contains_int(intptr_t arr, intptr_t value) {
    if (!arr) return 0;
    AymArray *a = (AymArray*)arr;
    for (long i = 0; i < a->len; i++) {
        if (a->data[i] == value) return 1;
    }
    return 0;
}

long aym_array_contains_str(intptr_t arr, const char *value) {
    if (!arr) return 0;
    AymArray *a = (AymArray*)arr;
    for (long i = 0; i < a->len; i++) {
        const char *item = (const char *)a->data[i];
        if (!item && !value) return 1;
        if (!item || !value) continue;
        if (strcmp(item, value) == 0) return 1;
    }
    return 0;
}

long aym_array_find_int(intptr_t arr, intptr_t value) {
    if (!arr) return -1;
    AymArray *a = (AymArray*)arr;
    for (long i = 0; i < a->len; i++) {
        if (a->data[i] == value) return i;
    }
    return -1;
}

long aym_array_find_str(intptr_t arr, const char *value) {
    if (!arr) return -1;
    AymArray *a = (AymArray*)arr;
    for (long i = 0; i < a->len; i++) {
        const char *item = (const char *)a->data[i];
        if (!item && !value) return i;
        if (!item || !value) continue;
        if (strcmp(item, value) == 0) return i;
    }
    return -1;
}

static int aym_cmp_intptr(const void *left, const void *right) {
    intptr_t a = *(const intptr_t *)left;
    intptr_t b = *(const intptr_t *)right;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static int aym_cmp_cstr_ptr(const void *left, const void *right) {
    const char *a = (const char *)(*(const intptr_t *)left);
    const char *b = (const char *)(*(const intptr_t *)right);
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    return strcmp(a, b);
}

intptr_t aym_array_sort_int(intptr_t arr) {
    if (!arr) return aym_array_new(0);
    AymArray *a = (AymArray*)arr;
    intptr_t out = aym_array_new(a->len);
    if (!out) return 0;
    AymArray *dst = (AymArray*)out;
    for (long i = 0; i < a->len; i++) {
        dst->data[i] = a->data[i];
    }
    if (dst->len > 1) {
        qsort(dst->data, (size_t)dst->len, sizeof(intptr_t), aym_cmp_intptr);
    }
    return out;
}

intptr_t aym_array_sort_str(intptr_t arr) {
    if (!arr) return aym_array_new(0);
    AymArray *a = (AymArray*)arr;
    intptr_t out = aym_array_new(a->len);
    if (!out) return 0;
    AymArray *dst = (AymArray*)out;
    for (long i = 0; i < a->len; i++) {
        dst->data[i] = a->data[i];
    }
    if (dst->len > 1) {
        qsort(dst->data, (size_t)dst->len, sizeof(intptr_t), aym_cmp_cstr_ptr);
    }
    return out;
}

intptr_t aym_array_unique_int(intptr_t arr) {
    if (!arr) return aym_array_new(0);
    AymArray *a = (AymArray*)arr;
    intptr_t out = aym_array_new(0);
    if (!out) return 0;
    for (long i = 0; i < a->len; i++) {
        if (!aym_array_contains_int(out, a->data[i])) {
            if (!aym_array_push(out, a->data[i])) {
                return out;
            }
        }
    }
    return out;
}

intptr_t aym_array_unique_str(intptr_t arr) {
    if (!arr) return aym_array_new(0);
    AymArray *a = (AymArray*)arr;
    intptr_t out = aym_array_new(0);
    if (!out) return 0;
    for (long i = 0; i < a->len; i++) {
        const char *value = (const char *)a->data[i];
        if (!aym_array_contains_str(out, value)) {
            if (!aym_array_push(out, (intptr_t)value)) {
                return out;
            }
        }
    }
    return out;
}

typedef struct {
    long len;
    long cap;
    char **keys;
    intptr_t *values;
    unsigned char *types;
} AymMap;

static long aym_map_find(AymMap *map, const char *key) {
    if (!map || !key) return -1;
    for (long i = 0; i < map->len; i++) {
        if (map->keys[i] && strcmp(map->keys[i], key) == 0) return i;
    }
    return -1;
}

intptr_t aym_map_new(long size) {
    if (size < 0) return 0;
    AymMap *map = calloc(1, sizeof(AymMap));
    if (!map) {
        fprintf(stderr, "aym_map_new: allocation failed\n");
        return 0;
    }
    map->len = 0;
    map->cap = size;
    if (size > 0) {
        map->keys = calloc((size_t)size, sizeof(char *));
        map->values = calloc((size_t)size, sizeof(intptr_t));
        map->types = calloc((size_t)size, sizeof(unsigned char));
        if (!map->keys || !map->values || !map->types) {
            fprintf(stderr, "aym_map_new: allocation failed\n");
            free(map->keys);
            free(map->values);
            free(map->types);
            free(map);
            return 0;
        }
    }
    return (intptr_t)map;
}

static int aym_map_grow(AymMap *map) {
    long newCap = map->cap > 0 ? map->cap * 2 : 1;
    char **newKeys = realloc(map->keys, sizeof(char *) * (size_t)newCap);
    intptr_t *newValues = realloc(map->values, sizeof(intptr_t) * (size_t)newCap);
    unsigned char *newTypes = realloc(map->types, sizeof(unsigned char) * (size_t)newCap);
    if (!newKeys || !newValues || !newTypes) {
        fprintf(stderr, "aym_map_set: allocation failed\n");
        free(newKeys);
        free(newValues);
        free(newTypes);
        return 0;
    }
    map->keys = newKeys;
    map->values = newValues;
    map->types = newTypes;
    map->cap = newCap;
    return 1;
}

long aym_map_size(intptr_t map) {
    if (!map) return 0;
    return ((AymMap*)map)->len;
}

long aym_map_contains(intptr_t map, const char *key) {
    if (!map || !key) return 0;
    return aym_map_find((AymMap*)map, key) >= 0;
}

intptr_t aym_map_set(intptr_t map, const char *key, intptr_t value, int is_string) {
    if (!map || !key) return 0;
    AymMap *m = (AymMap*)map;
    long idx = aym_map_find(m, key);
    if (idx >= 0) {
        m->values[idx] = (intptr_t)value;
        m->types[idx] = (unsigned char)(is_string ? 1 : 0);
        return value;
    }
    if (m->len >= m->cap) {
        if (!aym_map_grow(m)) return 0;
    }
    m->keys[m->len] = (char *)key;
    m->values[m->len] = (intptr_t)value;
    m->types[m->len] = (unsigned char)(is_string ? 1 : 0);
    m->len++;
    return value;
}

static void aym_map_missing_key(const char *key) {
    const char *suffix = key ? key : "";
    char *message = aym_str_concat("no existe: ", suffix);
    aym_throw_typed("CLAVE", message ? message : "no existe");
}

intptr_t aym_map_get(intptr_t map, const char *key) {
    if (!map) {
        aym_map_missing_key(key);
        return 0;
    }
    AymMap *m = (AymMap*)map;
    long idx = aym_map_find(m, key);
    if (idx < 0) {
        aym_map_missing_key(key);
        return 0;
    }
    return m->values[idx];
}

intptr_t aym_map_get_default(intptr_t map, const char *key, intptr_t default_value) {
    if (!map || !key) return default_value;
    AymMap *m = (AymMap*)map;
    long idx = aym_map_find(m, key);
    if (idx < 0) return default_value;
    return m->values[idx];
}

intptr_t aym_map_delete(intptr_t map, const char *key) {
    if (!map) {
        aym_map_missing_key(key);
        return 0;
    }
    AymMap *m = (AymMap*)map;
    long idx = aym_map_find(m, key);
    if (idx < 0) {
        aym_map_missing_key(key);
        return 0;
    }
    intptr_t value = m->values[idx];
    for (long i = idx + 1; i < m->len; i++) {
        m->keys[i - 1] = m->keys[i];
        m->values[i - 1] = m->values[i];
        m->types[i - 1] = m->types[i];
    }
    m->len--;
    return value;
}

intptr_t aym_map_keys(intptr_t map) {
    if (!map) return 0;
    AymMap *m = (AymMap*)map;
    intptr_t arr = aym_array_new(m->len);
    for (long i = 0; i < m->len; i++) {
        aym_array_set(arr, i, (intptr_t)m->keys[i]);
    }
    return arr;
}

intptr_t aym_map_values(intptr_t map) {
    if (!map) return 0;
    AymMap *m = (AymMap*)map;
    intptr_t arr = aym_array_new(m->len);
    for (long i = 0; i < m->len; i++) {
        aym_array_set(arr, i, (intptr_t)m->values[i]);
    }
    return arr;
}

const char *aym_map_key_at(intptr_t map, long idx) {
    if (!map) return "";
    AymMap *m = (AymMap*)map;
    if (idx < 0 || idx >= m->len) return "";
    return m->keys[idx] ? m->keys[idx] : "";
}

intptr_t aym_map_value_at(intptr_t map, long idx) {
    if (!map) return 0;
    AymMap *m = (AymMap*)map;
    if (idx < 0 || idx >= m->len) return 0;
    return m->values[idx];
}

long aym_map_value_is_string(intptr_t map, long idx) {
    if (!map) return 0;
    AymMap *m = (AymMap*)map;
    if (idx < 0 || idx >= m->len) return 0;
    return m->types[idx] ? 1 : 0;
}

long aym_map_value_is_string_key(intptr_t map, const char *key) {
    if (!map || !key) return 0;
    AymMap *m = (AymMap*)map;
    long idx = aym_map_find(m, key);
    if (idx < 0) return 0;
    return m->types[idx] ? 1 : 0;
}

char *aym_str_concat(const char *left, const char *right) {
    if (!left) left = "";
    if (!right) right = "";
    size_t left_len = strlen(left);
    size_t right_len = strlen(right);
    size_t total = left_len + right_len + 1;
    char *out = (char *)malloc(total);
    if (!out) return NULL;
    memcpy(out, left, left_len);
    memcpy(out + left_len, right, right_len);
    out[total - 1] = '\0';
    return out;
}

char *aym_str_trim(const char *text) {
    if (!text) text = "";
    const char *start = text;
    while (*start && isspace((unsigned char)*start)) start++;
    const char *end = text + strlen(text);
    while (end > start && isspace((unsigned char)*(end - 1))) end--;
    size_t len = (size_t)(end - start);
    return aym_str_copy(start, len);
}

intptr_t aym_str_split(const char *text, const char *sep) {
    if (!text) text = "";
    if (!sep || sep[0] == '\0') {
        aym_throw_typed("ARG", "separador vacio");
        return 0;
    }
    size_t sep_len = strlen(sep);
    if (*text == '\0') {
        intptr_t arr = aym_array_new(1);
        char *empty = aym_str_copy("", 0);
        if (empty) aym_array_set(arr, 0, (intptr_t)empty);
        return arr;
    }
    size_t count = 1;
    const char *scan = text;
    while ((scan = strstr(scan, sep)) != NULL) {
        count++;
        scan += sep_len;
    }
    intptr_t arr = aym_array_new((long)count);
    const char *start = text;
    size_t idx = 0;
    while (1) {
        const char *pos = strstr(start, sep);
        if (!pos) {
            char *piece = aym_str_copy(start, strlen(start));
            if (piece) aym_array_set(arr, (long)idx, (intptr_t)piece);
            break;
        }
        char *piece = aym_str_copy(start, (size_t)(pos - start));
        if (piece) aym_array_set(arr, (long)idx, (intptr_t)piece);
        idx++;
        start = pos + sep_len;
    }
    return arr;
}

char *aym_str_join(intptr_t arr, const char *sep) {
    if (!sep) sep = "";
    size_t sep_len = strlen(sep);
    if (!arr) return aym_str_copy("", 0);
    AymArray *a = (AymArray*)arr;
    if (a->len <= 0) return aym_str_copy("", 0);
    size_t total = 0;
    for (long i = 0; i < a->len; i++) {
        const char *part = (const char *)a->data[i];
        if (part) total += strlen(part);
        if (i + 1 < a->len) total += sep_len;
    }
    char *out = (char *)malloc(total + 1);
    if (!out) return NULL;
    char *cursor = out;
    for (long i = 0; i < a->len; i++) {
        const char *part = (const char *)a->data[i];
        if (part) {
            size_t len = strlen(part);
            memcpy(cursor, part, len);
            cursor += len;
        }
        if (i + 1 < a->len && sep_len > 0) {
            memcpy(cursor, sep, sep_len);
            cursor += sep_len;
        }
    }
    *cursor = '\0';
    return out;
}

char *aym_str_replace(const char *text, const char *search, const char *replacement) {
    if (!text) text = "";
    if (!search || search[0] == '\0') {
        aym_throw_typed("ARG", "busqueda vacia");
        return NULL;
    }
    if (!replacement) replacement = "";
    size_t search_len = strlen(search);
    size_t replacement_len = strlen(replacement);
    size_t count = 0;
    const char *scan = text;
    while ((scan = strstr(scan, search)) != NULL) {
        int start_ok = (scan == text) || !aym_is_word_char(*(scan - 1));
        int end_ok = !aym_is_word_char(scan[search_len]);
        if (start_ok && end_ok) {
            count++;
            scan += search_len;
        } else {
            scan += 1;
        }
    }
    if (count == 0) return aym_str_copy(text, strlen(text));
    size_t total = strlen(text) + count * (replacement_len - search_len);
    char *out = (char *)malloc(total + 1);
    if (!out) return NULL;
    const char *src = text;
    char *dst = out;
    while ((scan = strstr(src, search)) != NULL) {
        int start_ok = (scan == text) || !aym_is_word_char(*(scan - 1));
        int end_ok = !aym_is_word_char(scan[search_len]);
        if (!start_ok || !end_ok) {
            size_t chunk = (size_t)(scan - src + 1);
            memcpy(dst, src, chunk);
            dst += chunk;
            src = scan + 1;
            continue;
        }
        size_t chunk = (size_t)(scan - src);
        memcpy(dst, src, chunk);
        dst += chunk;
        if (replacement_len > 0) {
            memcpy(dst, replacement, replacement_len);
            dst += replacement_len;
        }
        src = scan + search_len;
    }
    size_t tail = strlen(src);
    memcpy(dst, src, tail);
    dst += tail;
    *dst = '\0';
    return out;
}

long aym_str_contains(const char *text, const char *sub) {
    if (!text) return 0;
    if (!sub) return 0;
    if (sub[0] == '\0') return 1;
    return strstr(text, sub) != NULL;
}

char *aym_to_string(long value) {
    char buffer[64];
    int written = snprintf(buffer, sizeof(buffer), "%ld", value);
    if (written < 0) return NULL;
    size_t len = (size_t)written;
    char *out = (char *)malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, buffer, len);
    out[len] = '\0';
    return out;
}

long aym_to_number(const char *text) {
    if (!text) {
        aym_throw_typed("CONVERSION", "texto vacio");
        return 0;
    }
    char *end = NULL;
    long value = strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        aym_throw_typed("CONVERSION", "no es numero");
        return 0;
    }
    return value;
}

typedef struct {
    const char *type;
    const char *message;
} AymException;

typedef struct AymHandler {
    jmp_buf env;
    struct AymHandler *prev;
    AymException *exception;
} AymHandler;

static AymHandler *current_handler = NULL;

intptr_t aym_exception_new(const char *type, const char *message) {
    AymException *exc = (AymException*)calloc(1, sizeof(AymException));
    if (!exc) return 0;
    exc->type = type ? type : "Error";
    exc->message = message ? message : "";
    return (intptr_t)exc;
}

const char *aym_exception_type(intptr_t exc) {
    if (!exc) return "";
    return ((AymException*)exc)->type;
}

const char *aym_exception_message(intptr_t exc) {
    if (!exc) return "";
    return ((AymException*)exc)->message;
}

intptr_t aym_try_push(void) {
    AymHandler *handler = (AymHandler*)calloc(1, sizeof(AymHandler));
    if (!handler) return 0;
    handler->prev = current_handler;
    current_handler = handler;
    return (intptr_t)handler;
}

void aym_try_pop(intptr_t handler) {
    if (!handler) return;
    AymHandler *h = (AymHandler*)handler;
    if (current_handler == h) {
        current_handler = h->prev;
    }
    free(h);
}

int aym_try_enter(intptr_t handler) {
    if (!handler) return 0;
    AymHandler *h = (AymHandler*)handler;
    return setjmp(h->env);
}

intptr_t aym_try_get_exception(intptr_t handler) {
    if (!handler) return 0;
    AymHandler *h = (AymHandler*)handler;
    return (intptr_t)h->exception;
}

intptr_t aym_try_env(intptr_t handler) {
    if (!handler) return 0;
    AymHandler *h = (AymHandler*)handler;
    return (intptr_t)h->env;
}

void aym_throw(intptr_t exception) {
    if (!current_handler) {
        fprintf(stderr, "Excepcion no manejada\n");
        exit(1);
    }
    current_handler->exception = (AymException*)exception;
    longjmp(current_handler->env, 1);
}
