#ifndef _WIN32
#define _GNU_SOURCE /* pthread_getattr_np */
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <stdint.h>
#include <setjmp.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <termios.h>
#include <signal.h>
#include <sys/resource.h>
#include <pthread.h>
#include <fcntl.h>
#endif

/* Ubicacion actual del programa AymaraLang (la actualiza el codigo generado
 * en cada sentencia) para los mensajes de error en ejecucion. */
long long aym_cur_line = 0;
const char *aym_cur_file = NULL;

intptr_t aym_exception_new(const char *type, const char *message);
void aym_throw(intptr_t exception);
intptr_t aym_array_new(long long size);
void aym_print_float(int64_t bits);
intptr_t aym_array_get(intptr_t arr, long long idx);
long long aym_array_length(intptr_t arr);
intptr_t aym_array_push(intptr_t arr, intptr_t val);
long long aym_gfx_set_color(long long r, long long g, long long b);
long long aym_gfx_rect4(long long x, long long y, long long w, long long h);
long long aym_gfx_text3(const char *text, long long x, long long y);
#ifndef _WIN32
long aym_linux_gfx_open(long width, long height, const char *title);
long aym_linux_gfx_is_open(void);
long aym_linux_gfx_clear(long r, long g, long b);
long aym_linux_gfx_set_color(long r, long g, long b);
long aym_linux_gfx_rect4(long x, long y, long w, long h);
long aym_linux_gfx_text3(const char *text, long x, long y);
long aym_linux_gfx_present(void);
long aym_linux_gfx_close(void);
long aym_linux_gfx_key_down(long key);
#endif

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

static long long aym_cli_argc = 0;
static const char **aym_cli_argv = NULL;

/* Fallos fatales (desbordamiento de pila por recursion infinita, acceso
 * invalido a memoria): sin manejador el proceso muere sin vaciar stdout y el
 * usuario pierde la salida previa. Se vacia stdout y se explica el error. */
#ifdef _WIN32
static LONG WINAPI aym_fatal_filter(EXCEPTION_POINTERS *info) {
    DWORD code = info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0;
    fflush(stdout);
    if (code == EXCEPTION_STACK_OVERFLOW) {
        fputs("Error fatal: desbordamiento de pila (recursion demasiado profunda o infinita)\n", stderr);
    } else {
        fputs("Error fatal: acceso invalido a memoria\n", stderr);
    }
    if (aym_cur_line > 0) {
        fprintf(stderr, "  en %s, linea %lld\n",
                (aym_cur_file && aym_cur_file[0]) ? aym_cur_file : "programa", aym_cur_line);
    }
    fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}

static void aym_install_fatal_handlers(void) {
    ULONG guarantee = 16 * 1024;
    SetThreadStackGuarantee(&guarantee);
    SetUnhandledExceptionFilter(aym_fatal_filter);
}
#else
static void aym_fatal_signal(int sig) {
    static const char overflow[] =
        "Error fatal: desbordamiento de pila o acceso invalido a memoria "
        "(revisa recursion demasiado profunda o infinita)\n";
    static const char other[] = "Error fatal: senal inesperada\n";
    /* fflush/snprintf no son async-signal-safe, pero el proceso va a
     * terminar y perder la salida es peor; en la practica stdout no esta a
     * medio usar. */
    fflush(stdout);
    if (sig == SIGSEGV || sig == SIGBUS) {
        (void)!write(2, overflow, sizeof(overflow) - 1);
    } else {
        (void)!write(2, other, sizeof(other) - 1);
    }
    if (aym_cur_line > 0) {
        char where[512];
        int n = snprintf(where, sizeof(where), "  en %s, linea %lld\n",
                         (aym_cur_file && aym_cur_file[0]) ? aym_cur_file : "programa",
                         aym_cur_line);
        if (n > 0) (void)!write(2, where, (size_t)(n < (int)sizeof(where) ? n : (int)sizeof(where) - 1));
    }
    _exit(128 + sig);
}

static void aym_install_fatal_handlers(void) {
    static char altstack[64 * 1024];
    stack_t ss;
    ss.ss_sp = altstack;
    ss.ss_size = sizeof(altstack);
    ss.ss_flags = 0;
    sigaltstack(&ss, NULL);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = aym_fatal_signal;
    sa.sa_flags = SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
}
#endif

/* Limite de pila: cada funcion generada compara rsp con aym_stack_limit al
 * entrar y, si lo cruza, llama a aym_stack_overflow, que lanza la excepcion
 * PILA (atrapable y con ubicacion). Se deja un margen para que el lanzamiento
 * y el mensaje tengan pila de sobra; no depende de senales del sistema. */
uintptr_t aym_stack_limit = 0;

void aym_stack_overflow(void) {
    aym_throw_typed("PILA", "desbordamiento de pila (recursion demasiado profunda o infinita)");
}

static void aym_init_stack_limit(void) {
    const uintptr_t margin = 256 * 1024;
#ifdef _WIN32
    ULONG_PTR low = 0, high = 0;
    GetCurrentThreadStackLimits(&low, &high);
    if (low) aym_stack_limit = (uintptr_t)low + margin;
#else
    /* Limites exactos de la pila del hilo principal. */
    pthread_attr_t attr;
    if (pthread_getattr_np(pthread_self(), &attr) == 0) {
        void *addr = NULL;
        size_t stack_size = 0;
        int ok = pthread_attr_getstack(&attr, &addr, &stack_size) == 0;
        pthread_attr_destroy(&attr);
        if (ok && addr && stack_size > margin * 2) {
            aym_stack_limit = (uintptr_t)addr + margin;
            return;
        }
    }
    /* Respaldo: estimar desde una variable local y RLIMIT_STACK. */
    char here;
    uintptr_t base = (uintptr_t)&here;
    struct rlimit rl;
    uintptr_t size = 8u * 1024 * 1024;
    if (getrlimit(RLIMIT_STACK, &rl) == 0) {
        if (rl.rlim_cur == RLIM_INFINITY) size = (uintptr_t)1024 * 1024 * 1024;
        else size = (uintptr_t)rl.rlim_cur;
    }
    /* base esta cerca del tope de la pila (main acaba de empezar). */
    if (size > margin * 2 && base > size) aym_stack_limit = base - size + margin;
#endif
}

void aym_set_args(long long argc, const char **argv) {
    aym_cli_argc = argc;
    aym_cli_argv = argv;
    aym_install_fatal_handlers();
    aym_init_stack_limit();
}

long long aym_argc(void) {
    return aym_cli_argc;
}

const char *aym_argv_get(long long idx) {
    if (!aym_cli_argv) return "";
    if (idx < 0 || idx >= aym_cli_argc) return "";
    if (!aym_cli_argv[idx]) return "";
    return aym_cli_argv[idx];
}

void aym_exit(long long code) {
    fflush(stdout);
    exit((int)code);
}

long long aym_math_abs(long long x) {
    if (x < 0) {
        return (x == LLONG_MIN) ? LLONG_MAX : -x;
    }
    return x;
}

long long aym_math_min(long long a, long long b) {
    return (a < b) ? a : b;
}

long long aym_math_max(long long a, long long b) {
    return (a > b) ? a : b;
}

long long aym_math_gcd(long long a, long long b) {
    unsigned long long ua = (a < 0) ? (0ULL - (unsigned long long)a) : (unsigned long long)a;
    unsigned long long ub = (b < 0) ? (0ULL - (unsigned long long)b) : (unsigned long long)b;
    while (ub != 0UL) {
        unsigned long long r = ua % ub;
        ua = ub;
        ub = r;
    }
    if (ua > (unsigned long long)LLONG_MAX) return LLONG_MAX;
    return (long long)ua;
}

long long aym_math_clamp(long long x, long long lo, long long hi) {
    if (lo > hi) {
        long long tmp = lo;
        lo = hi;
        hi = tmp;
    }
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

long long aym_assert(long long cond, const char *message) {
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
    long long len = aym_array_length(arr);
    intptr_t out = aym_array_new(0);
    AymUnaryFn fn = (AymUnaryFn)fn_ptr;
    for (long long i = 0; i < len; i++) {
        intptr_t v = aym_array_get(arr, i);
        intptr_t mapped = fn(v);
        aym_array_push(out, mapped);
    }
    return out;
}

intptr_t aym_hof_filter(intptr_t arr, intptr_t fn_ptr) {
    if (!arr || !fn_ptr) return aym_array_new(0);
    long long len = aym_array_length(arr);
    intptr_t out = aym_array_new(0);
    AymUnaryFn fn = (AymUnaryFn)fn_ptr;
    for (long long i = 0; i < len; i++) {
        intptr_t v = aym_array_get(arr, i);
        if (fn(v)) {
            aym_array_push(out, v);
        }
    }
    return out;
}

intptr_t aym_hof_reduce(intptr_t arr, intptr_t fn_ptr, intptr_t init) {
    if (!arr || !fn_ptr) return init;
    long long len = aym_array_length(arr);
    intptr_t acc = init;
    AymBinaryFn fn = (AymBinaryFn)fn_ptr;
    for (long long i = 0; i < len; i++) {
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
    long long size = ftell(f);
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

static long long aym_fs_write_mode(const char *path, const char *text, const char *mode) {
    if (!path || !path[0]) return 0;
    FILE *f = fopen(path, mode);
    if (!f) return 0;
    if (!text) text = "";
    size_t len = strlen(text);
    size_t written = fwrite(text, 1, len, f);
    int closed = fclose(f);
    return (written == len && closed == 0) ? 1 : 0;
}

long long aym_fs_write_text(const char *path, const char *text) {
    return aym_fs_write_mode(path, text, "wb");
}

long long aym_fs_append_text(const char *path, const char *text) {
    return aym_fs_write_mode(path, text, "ab");
}

intptr_t aym_fs_read_lines(const char *path) {
    intptr_t arr = aym_array_new(0);
    if (!arr || !path || !path[0]) return arr;
    FILE *f = fopen(path, "rb");
    if (!f) return arr;
    size_t cap = 128;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) {
        fclose(f);
        return arr;
    }
    int ch = 0;
    while ((ch = fgetc(f)) != EOF) {
        if (ch == '\n') {
            while (len > 0 && buf[len - 1] == '\r') {
                len--;
            }
            char *line = aym_str_copy(buf, len);
            if (line) aym_array_push(arr, (intptr_t)line);
            len = 0;
            continue;
        }
        if (len + 1 >= cap) {
            size_t next_cap = cap * 2;
            char *next = (char *)realloc(buf, next_cap);
            if (!next) {
                free(buf);
                fclose(f);
                return arr;
            }
            buf = next;
            cap = next_cap;
        }
        buf[len++] = (char)ch;
    }
    if (len > 0) {
        while (len > 0 && buf[len - 1] == '\r') {
            len--;
        }
        char *line = aym_str_copy(buf, len);
        if (line) aym_array_push(arr, (intptr_t)line);
    }
    free(buf);
    fclose(f);
    return arr;
}

long long aym_fs_delete(const char *path) {
    if (!path || !path[0]) return 0;
    return remove(path) == 0 ? 1 : 0;
}

long long aym_fs_exists(const char *path) {
    if (!path || !path[0]) return 0;
    struct stat st;
    return stat(path, &st) == 0 ? 1 : 0;
}

char *aym_env_get(const char *name) {
    if (!name || !name[0]) return aym_str_copy("", 0);
    const char *val = getenv(name);
    if (!val) return aym_str_copy("", 0);
    return aym_str_copy(val, strlen(val));
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

void aym_term_move(long long x, long long y) {
    aym_term_init();
    if (x < 1) x = 1;
    if (y < 1) y = 1;
    printf("\x1b[%lld;%lldH", y, x);
    fflush(stdout);
}

void aym_term_color(long long fg, long long bg) {
    aym_term_init();
    if (fg < 0) fg = 7;
    if (bg < 0) bg = 0;
    if (fg > 15) fg = 15;
    if (bg > 15) bg = 15;
    long long fgCode = (fg < 8) ? (30 + fg) : (90 + (fg - 8));
    long long bgCode = (bg < 8) ? (40 + bg) : (100 + (bg - 8));
    printf("\x1b[%lld;%lldm", fgCode, bgCode);
    fflush(stdout);
}

void aym_term_reset(void) {
    aym_term_init();
    printf("\x1b[0m");
    fflush(stdout);
}

void aym_term_cursor(long long visible) {
    aym_term_init();
    printf(visible ? "\x1b[?25h" : "\x1b[?25l");
    fflush(stdout);
}

#ifdef _WIN32
static int aym_key_down(int vk) {
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

static long long aym_key_ascii(void) {
    for (int vk = 'A'; vk <= 'Z'; ++vk) {
        if (aym_key_down(vk)) return (long long)(vk + ('a' - 'A'));
    }
    for (int vk = '0'; vk <= '9'; ++vk) {
        if (aym_key_down(vk)) return (long long)vk;
    }
    if (aym_key_down(VK_SPACE)) return (long long)' ';
    if (aym_key_down(VK_RETURN)) return (long long)'\r';
    if (aym_key_down(VK_ESCAPE)) return 27;
    if (aym_key_down(VK_BACK)) return 8;
    if (aym_key_down(VK_TAB)) return (long long)'\t';
    return -1;
}
#endif

long long aym_key_poll(void) {
    aym_term_init();
#ifdef _WIN32
    if (aym_key_down(VK_LEFT)) return 1003;
    if (aym_key_down(VK_RIGHT)) return 1004;
    if (aym_key_down(VK_DOWN)) return 1002;
    if (aym_key_down(VK_UP)) return 1001;
    long long ascii = aym_key_ascii();
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
    return (long long)ch;
#endif
}

long long aym_time_ms(void) {
#ifdef _WIN32
    return (long long)GetTickCount64();
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) return 0;
    return (long long)(tv.tv_sec * 1000L + tv.tv_usec / 1000L);
#endif
}

#ifdef _WIN32
static long long aym_color_clip(long long value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return value;
}

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
static long long aym_gfx_width = 0;
static long long aym_gfx_height = 0;
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

static int aym_gfx_key_to_vk(long long key) {
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

long long aym_gfx_open(long long width, long long height, const char *title) {
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
    return aym_linux_gfx_open(width, height, title);
#endif
}

long long aym_gfx_is_open(void) {
#ifdef _WIN32
    aym_gfx_pump_messages();
    return aym_gfx_alive ? 1 : 0;
#else
    return aym_linux_gfx_is_open();
#endif
}

long long aym_gfx_clear(long long r, long long g, long long b) {
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
    return aym_linux_gfx_clear(r, g, b);
#endif
}

long long aym_gfx_rect(long long x, long long y, long long w, long long h, long long r, long long g, long long b) {
#ifdef _WIN32
    aym_gfx_current_color = ((uint32_t)aym_color_clip(r) << 16) |
                            ((uint32_t)aym_color_clip(g) << 8) |
                            (uint32_t)aym_color_clip(b);
    return aym_gfx_rect4(x, y, w, h);
#else
    if (!aym_linux_gfx_set_color(r, g, b)) return 0;
    return aym_linux_gfx_rect4(x, y, w, h);
#endif
}

long long aym_gfx_set_color(long long r, long long g, long long b) {
#ifdef _WIN32
    aym_gfx_current_color = ((uint32_t)aym_color_clip(r) << 16) |
                            ((uint32_t)aym_color_clip(g) << 8) |
                            (uint32_t)aym_color_clip(b);
    return 1;
#else
    return aym_linux_gfx_set_color(r, g, b);
#endif
}

long long aym_gfx_rect4(long long x, long long y, long long w, long long h) {
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

    long long x0 = x < 0 ? 0 : x;
    long long y0 = y < 0 ? 0 : y;
    long long x1 = x + w;
    long long y1 = y + h;
    if (x1 > aym_gfx_width) x1 = aym_gfx_width;
    if (y1 > aym_gfx_height) y1 = aym_gfx_height;
    if (x0 >= x1 || y0 >= y1) return 1;

    for (long long yy = y0; yy < y1; ++yy) {
        size_t row = (size_t)yy * (size_t)aym_gfx_width;
        for (long long xx = x0; xx < x1; ++xx) {
            aym_gfx_pixels[row + (size_t)xx] = aym_gfx_current_color;
        }
    }
    return 1;
#else
    return aym_linux_gfx_rect4(x, y, w, h);
#endif
}

long long aym_gfx_text(const char *text, long long x, long long y, long long r, long long g, long long b) {
#ifdef _WIN32
    aym_gfx_current_color = ((uint32_t)aym_color_clip(r) << 16) |
                            ((uint32_t)aym_color_clip(g) << 8) |
                            (uint32_t)aym_color_clip(b);
    return aym_gfx_text3(text, x, y);
#else
    if (!aym_linux_gfx_set_color(r, g, b)) return 0;
    return aym_linux_gfx_text3(text, x, y);
#endif
}

long long aym_gfx_text3(const char *text, long long x, long long y) {
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
    return aym_linux_gfx_text3(text, x, y);
#endif
}

long long aym_gfx_present(void) {
#ifdef _WIN32
    if (!aym_gfx_alive || !aym_gfx_window_dc || !aym_gfx_mem_dc) return 0;
    aym_gfx_pump_messages();
    if (!aym_gfx_alive) return 0;
    return BitBlt(
        aym_gfx_window_dc, 0, 0, (int)aym_gfx_width, (int)aym_gfx_height,
        aym_gfx_mem_dc, 0, 0, SRCCOPY) ? 1 : 0;
#else
    return aym_linux_gfx_present();
#endif
}

long long aym_gfx_close(void) {
#ifdef _WIN32
    aym_gfx_close_window();
    return 1;
#else
    return aym_linux_gfx_close();
#endif
}

long long aym_gfx_key_down(long long key) {
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
    return aym_linux_gfx_key_down(key);
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

long long aym_file_write_text(const char *path, const char *text) {
    if (!path || path[0] == '\0') return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    if (!text) text = "";
    size_t len = strlen(text);
    size_t written = fwrite(text, 1, len, f);
    fclose(f);
    return (written == len) ? 1 : 0;
}

long long aym_record_load(void) {
    FILE *f = fopen("tetris_highscore.txt", "rb");
    if (!f) return 0;
    long long value = 0;
    if (fscanf(f, "%lld", &value) != 1) value = 0;
    fclose(f);
    return value;
}

long long aym_record_save(long long value) {
    FILE *f = fopen("tetris_highscore.txt", "wb");
    if (!f) return 0;
    int ok = fprintf(f, "%lld", value);
    fclose(f);
    return ok > 0 ? 1 : 0;
}

#include "runtime_arrays.c"

#include "runtime_maps_strings.c"

#include "runtime_exceptions.c"
