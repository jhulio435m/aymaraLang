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
