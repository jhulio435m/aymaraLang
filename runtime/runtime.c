#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

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

