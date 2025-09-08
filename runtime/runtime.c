#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
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

int aym_random(int max) {
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = 1;
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

intptr_t aym_array_new(long size) {
    if (size <= 0) return 0;
    // allocate extra slot to store array length
    long *arr = calloc((size_t)size + 1, sizeof(long));
    if (!arr) {
        fprintf(stderr, "aym_array_new: allocation failed\n");
        return 0;
    }
    arr[0] = size;            // store length at the first position
    return (intptr_t)(arr + 1); // return pointer to data region
}

long aym_array_get(intptr_t arr, long idx) {
    if (!arr) return 0;
    long *a = (long*)arr;
    long len = *(a - 1);
    if (idx < 0 || idx >= len) return 0;
    return a[idx];
}

long aym_array_set(intptr_t arr, long idx, long val) {
    if (!arr) return 0;
    long *a = (long*)arr;
    long len = *(a - 1);
    if (idx < 0 || idx >= len) return 0;
    a[idx] = val;
    return val;
}

void aym_array_free(intptr_t arr) {
    if (!arr) return;
    long *a = (long*)arr;
    long *base = a - 1;
    base[0] = 0; // clear length bookkeeping
    free(base);
}

