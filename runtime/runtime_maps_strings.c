typedef struct {
    long len;
    long cap;
    char **keys;
    intptr_t *values;
    unsigned char *types;
    long *buckets;
    long bucket_cap;
} AymMap;

static uint64_t aym_map_hash(const char *s) {
    uint64_t h = UINT64_C(14695981039346656037);
    if (!s) return h;
    for (const unsigned char *p = (const unsigned char *)s; *p; ++p) {
        h ^= (uint64_t)(*p);
        h *= UINT64_C(1099511628211);
    }
    return h;
}

static int aym_map_rehash(AymMap *m, long new_bcap) {
    if (!m) return 0;
    long bcap = 16;
    while (bcap < new_bcap && bcap <= LONG_MAX / 2) {
        bcap *= 2;
    }
    long *target = NULL;
    int reused = 0;
    if (m->buckets && m->bucket_cap == bcap) {
        target = m->buckets;
        reused = 1;
    } else {
        target = (long *)malloc(sizeof(long) * (size_t)bcap);
        if (!target) {
            fprintf(stderr, "aym_map_rehash: allocation failed\n");
            return 0;
        }
    }
    for (long i = 0; i < bcap; i++) {
        target[i] = -1;
    }
    uint64_t mask = (uint64_t)(bcap - 1);
    for (long i = 0; i < m->len; i++) {
        if (!m->keys[i]) continue;
        uint64_t h = aym_map_hash(m->keys[i]);
        for (long step = 0; step < bcap; step++) {
            long slot = (long)((h + (uint64_t)step) & mask);
            if (target[slot] < 0) {
                target[slot] = i;
                break;
            }
        }
    }
    if (!reused) {
        free(m->buckets);
        m->buckets = target;
        m->bucket_cap = bcap;
    }
    return 1;
}

static long aym_map_find(AymMap *map, const char *key) {
    if (!map || !key || !map->buckets || map->bucket_cap <= 0 || map->len <= 0) return -1;
    uint64_t h = aym_map_hash(key);
    uint64_t mask = (uint64_t)(map->bucket_cap - 1);
    for (long step = 0; step < map->bucket_cap; step++) {
        long slot = (long)((h + (uint64_t)step) & mask);
        long idx = map->buckets[slot];
        if (idx < 0) return -1;
        if (idx < map->len && map->keys[idx] && strcmp(map->keys[idx], key) == 0) {
            return idx;
        }
    }
    return -1;
}

intptr_t aym_map_new(long size) {
    if (size < 0) return 0;
    AymMap *map = (AymMap *)calloc(1, sizeof(AymMap));
    if (!map) {
        fprintf(stderr, "aym_map_new: allocation failed\n");
        return 0;
    }
    map->len = 0;
    map->cap = size;
    long bcap = 16;
    while (bcap / 2 < size && bcap <= LONG_MAX / 2) {
        bcap *= 2;
    }
    map->buckets = (long *)malloc(sizeof(long) * (size_t)bcap);
    if (!map->buckets) {
        fprintf(stderr, "aym_map_new: allocation failed\n");
        free(map);
        return 0;
    }
    for (long i = 0; i < bcap; i++) {
        map->buckets[i] = -1;
    }
    map->bucket_cap = bcap;
    if (size > 0) {
        map->keys = (char **)calloc((size_t)size, sizeof(char *));
        map->values = (intptr_t *)calloc((size_t)size, sizeof(intptr_t));
        map->types = (unsigned char *)calloc((size_t)size, sizeof(unsigned char));
        if (!map->keys || !map->values || !map->types) {
            fprintf(stderr, "aym_map_new: allocation failed\n");
            free(map->keys);
            free(map->values);
            free(map->types);
            free(map->buckets);
            free(map);
            return 0;
        }
    }
    return (intptr_t)map;
}

static int aym_map_grow(AymMap *map) {
    long newCap = map->cap > 0 ? map->cap * 2 : 1;
    char **newKeys = (char **)realloc(map->keys, sizeof(char *) * (size_t)newCap);
    if (!newKeys) {
        fprintf(stderr, "aym_map_set: allocation failed\n");
        return 0;
    }
    map->keys = newKeys;
    intptr_t *newValues = (intptr_t *)realloc(map->values, sizeof(intptr_t) * (size_t)newCap);
    if (!newValues) {
        fprintf(stderr, "aym_map_set: allocation failed\n");
        return 0;
    }
    map->values = newValues;
    unsigned char *newTypes = (unsigned char *)realloc(map->types, sizeof(unsigned char) * (size_t)newCap);
    if (!newTypes) {
        fprintf(stderr, "aym_map_set: allocation failed\n");
        return 0;
    }
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
        m->types[idx] = (unsigned char)((is_string == 1 || is_string == 2) ? is_string : 0);
        return value;
    }
    if (m->len >= m->cap) {
        if (!aym_map_grow(m)) return 0;
    }
    if (m->bucket_cap == 0 || (m->len + 1) * 2 > m->bucket_cap) {
        long next_bcap = m->bucket_cap > 0 ? m->bucket_cap * 2 : 16;
        if (next_bcap < 16) next_bcap = 16;
        if (!aym_map_rehash(m, next_bcap)) return 0;
    }
    long new_idx = m->len;
    m->keys[new_idx] = (char *)key;
    m->values[new_idx] = (intptr_t)value;
    m->types[new_idx] = (unsigned char)((is_string == 1 || is_string == 2) ? is_string : 0);
    m->len++;
    uint64_t h = aym_map_hash(key);
    uint64_t mask = (uint64_t)(m->bucket_cap - 1);
    for (long step = 0; step < m->bucket_cap; step++) {
        long slot = (long)((h + (uint64_t)step) & mask);
        if (m->buckets[slot] < 0) {
            m->buckets[slot] = new_idx;
            break;
        }
    }
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
    if (m->bucket_cap > 0) {
        aym_map_rehash(m, m->bucket_cap);
    }
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
    return m->types[idx] == 1 ? 1 : 0;
}

/* Tipo del valor: 0 = entero/otro, 1 = aru, 2 = chikata (bits de double). */
long aym_map_value_kind(intptr_t map, long idx) {
    if (!map) return 0;
    AymMap *m = (AymMap*)map;
    if (idx < 0 || idx >= m->len) return 0;
    return m->types[idx];
}

long aym_map_value_is_string_key(intptr_t map, const char *key) {
    if (!map || !key) return 0;
    AymMap *m = (AymMap*)map;
    long idx = aym_map_find(m, key);
    if (idx < 0) return 0;
    return m->types[idx] == 1 ? 1 : 0;
}

long aym_map_value_kind_key(intptr_t map, const char *key) {
    if (!map || !key) return 0;
    AymMap *m = (AymMap*)map;
    long idx = aym_map_find(m, key);
    if (idx < 0) return 0;
    return m->types[idx];
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

char *aym_str_upper(const char *text) {
    if (!text) text = "";
    size_t len = strlen(text);
    char *out = aym_str_copy(text, len);
    if (!out) return NULL;
    for (size_t i = 0; i < len; i++) out[i] = (char)toupper((unsigned char)out[i]);
    return out;
}

char *aym_str_lower(const char *text) {
    if (!text) text = "";
    size_t len = strlen(text);
    char *out = aym_str_copy(text, len);
    if (!out) return NULL;
    for (size_t i = 0; i < len; i++) out[i] = (char)tolower((unsigned char)out[i]);
    return out;
}

char *aym_str_reverse(const char *text) {
    if (!text) text = "";
    size_t len = strlen(text);
    char *out = aym_str_copy(text, len);
    if (!out) return NULL;
    size_t i = 0;
    size_t w = len;
    while (i < len) {
        size_t n = 1;
        while (i + n < len && (((unsigned char)text[i + n]) & 0xC0) == 0x80) n++;
        w -= n;
        memcpy(out + w, text + i, n);
        i += n;
    }
    return out;
}

char *aym_str_repeat(const char *text, long times) {
    if (!text) text = "";
    size_t len = strlen(text);
    if (times <= 0 || len == 0) return aym_str_copy("", 0);
    if ((size_t)times > ((size_t)1 << 30) / len) {
        aym_throw_typed("ARG", "repeticion demasiado grande");
        return aym_str_copy("", 0);
    }
    size_t total = len * (size_t)times;
    char *out = (char *)malloc(total + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < (size_t)times; i++) memcpy(out + i * len, text, len);
    out[total] = '\0';
    return out;
}

long aym_str_starts_with(const char *text, const char *prefix) {
    if (!text) text = "";
    if (!prefix) prefix = "";
    size_t plen = strlen(prefix);
    if (plen > strlen(text)) return 0;
    return memcmp(text, prefix, plen) == 0 ? 1 : 0;
}

long aym_str_ends_with(const char *text, const char *suffix) {
    if (!text) text = "";
    if (!suffix) suffix = "";
    size_t tlen = strlen(text);
    size_t slen = strlen(suffix);
    if (slen > tlen) return 0;
    return memcmp(text + (tlen - slen), suffix, slen) == 0 ? 1 : 0;
}

long aym_str_index_of(const char *text, const char *sub) {
    if (!text) text = "";
    if (!sub) sub = "";
    const char *pos = strstr(text, sub);
    if (!pos) return -1;
    return (long)(pos - text);
}

char *aym_str_char_at(const char *text, long idx) {
    if (!text) text = "";
    long len = (long)strlen(text);
    if (idx < 0 || idx >= len) {
        aym_throw_typed("INDICE", "indice de texto fuera de rango");
        return aym_str_copy("", 0);
    }
    return aym_str_copy(text + idx, 1);
}

char *aym_str_substr(const char *text, long start, long count) {
    if (!text) text = "";
    long len = (long)strlen(text);
    if (start < 0) start = 0;
    if (start > len) start = len;
    long avail = len - start;
    if (count < 0) count = 0;
    if (count > avail) count = avail;
    return aym_str_copy(text + start, (size_t)count);
}

long aym_str_ord(const char *text) {
    if (!text || text[0] == '\0') return -1;
    return (long)(unsigned char)text[0];
}

char *aym_str_chr(long code) {
    if (code < 0 || code > 255) {
        aym_throw_typed("ARG", "codigo de caracter fuera de rango");
        return aym_str_copy("", 0);
    }
    char *out = (char *)malloc(2);
    if (!out) return NULL;
    out[0] = (char)(unsigned char)code;
    out[1] = '\0';
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

/* ---- chikata (decimales) ----
 * Los decimales viajan como los 64 bits del double en un entero (intptr_t),
 * asi el codigo generado y el runtime no necesitan registros xmm en el ABI. */
static double aym_bits_to_double(int64_t bits) {
    double d;
    memcpy(&d, &bits, sizeof(d));
    return d;
}

static int64_t aym_double_to_bits(double d) {
    int64_t bits;
    memcpy(&bits, &d, sizeof(bits));
    return bits;
}

/* Formato legible y estable: 15 cifras significativas y siempre con parte
 * decimal ("3.0"), salvo notacion cientifica, inf y nan. */
static void aym_format_double(double v, char *buf, size_t n) {
    if (v != v) { snprintf(buf, n, "nan"); return; }
    if (v > 1.7976931348623157e308) { snprintf(buf, n, "inf"); return; }
    if (v < -1.7976931348623157e308) { snprintf(buf, n, "-inf"); return; }
    snprintf(buf, n, "%.15g", v);
    if (!strchr(buf, '.') && !strchr(buf, 'e') && strlen(buf) + 2 < n) {
        strcat(buf, ".0");
    }
}

void aym_print_float(int64_t bits) {
    char buf[64];
    aym_format_double(aym_bits_to_double(bits), buf, sizeof(buf));
    fputs(buf, stdout);
}

char *aym_float_to_str(int64_t bits) {
    char buf[64];
    aym_format_double(aym_bits_to_double(bits), buf, sizeof(buf));
    return aym_str_copy(buf, strlen(buf));
}

int64_t aym_str_to_float(const char *text) {
    if (!text || !*text) {
        aym_throw_typed("CONVERSION", "texto vacio");
        return 0;
    }
    char *end = NULL;
    double value = strtod(text, &end);
    if (end == text || *end != '\0') {
        aym_throw_typed("CONVERSION", "no es numero decimal");
        return 0;
    }
    return aym_double_to_bits(value);
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
