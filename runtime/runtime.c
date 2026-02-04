#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <setjmp.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

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

long aym_array_get(intptr_t arr, long idx) {
    if (!arr) return 0;
    AymArray *a = (AymArray*)arr;
    if (idx < 0) {
        fprintf(stderr, "aym_array_get: negative index %ld\n", idx);
        return 0; // default value on error
    }
    if (idx >= a->len) return 0;
    return (long)a->data[idx];
}

long aym_array_set(intptr_t arr, long idx, long val) {
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

intptr_t aym_array_push(intptr_t arr, long val) {
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
    a->data[a->len++] = (intptr_t)val;
    return arr;
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
    if (!text) return 0;
    return strtol(text, NULL, 10);
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

void aym_throw(intptr_t exception) {
    if (!current_handler) {
        fprintf(stderr, "Excepcion no manejada\n");
        exit(1);
    }
    current_handler->exception = (AymException*)exception;
    longjmp(current_handler->env, 1);
}
