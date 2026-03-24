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
