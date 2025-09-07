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
    long *arr = calloc((size_t)size, sizeof(long));
    return (intptr_t)arr;
}

long aym_array_get(intptr_t arr, long idx) {
    long *a = (long*)arr;
    return a[idx];
}

long aym_array_set(intptr_t arr, long idx, long val) {
    long *a = (long*)arr;
    a[idx] = val;
    return val;
}

