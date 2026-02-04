#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <setjmp.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

intptr_t aym_exception_new(const char *type, const char *message);
void aym_throw(intptr_t exception);

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
