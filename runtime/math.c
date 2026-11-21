#define _GNU_SOURCE
#define _ISOC99_SOURCE
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/* Los valores chikata llegan y salen como los 64 bits del double en un
 * entero, para no depender de registros xmm en la convencion de llamada. */
static double aym_d(int64_t bits) {
    double d;
    memcpy(&d, &bits, sizeof(d));
    return d;
}

static int64_t aym_b(double d) {
    int64_t bits;
    memcpy(&bits, &d, sizeof(bits));
    return bits;
}

/* Redondeo a jakhüwi con saturacion (nan -> 0). */
static long aym_math_to_long(double value) {
    if (value != value) return 0;
    if (value >= (double)LONG_MAX) return LONG_MAX;
    if (value <= (double)LONG_MIN) return LONG_MIN;
    return (long)value;
}

int64_t aym_sin(int64_t x)   { return aym_b(sin(aym_d(x))); }
int64_t aym_cos(int64_t x)   { return aym_b(cos(aym_d(x))); }
int64_t aym_tan(int64_t x)   { return aym_b(tan(aym_d(x))); }
int64_t aym_asin(int64_t x)  { return aym_b(asin(aym_d(x))); }
int64_t aym_acos(int64_t x)  { return aym_b(acos(aym_d(x))); }
int64_t aym_atan(int64_t x)  { return aym_b(atan(aym_d(x))); }
int64_t aym_sqrt(int64_t x)  { return aym_b(sqrt(aym_d(x))); }
int64_t aym_exp(int64_t x)   { return aym_b(exp(aym_d(x))); }
int64_t aym_log(int64_t x)   { return aym_b(log(aym_d(x))); }
int64_t aym_log10(int64_t x) { return aym_b(log10(aym_d(x))); }
int64_t aym_fabs(int64_t x)  { return aym_b(fabs(aym_d(x))); }

int64_t aym_pow(int64_t base, int64_t exponent) {
    return aym_b(pow(aym_d(base), aym_d(exponent)));
}

/* Operadores % y ^ con operandos chikata. */
int64_t aym_fmod(int64_t a, int64_t b) { return aym_b(fmod(aym_d(a), aym_d(b))); }
int64_t aym_fpow(int64_t a, int64_t b) { return aym_b(pow(aym_d(a), aym_d(b))); }

/* floor/ceil/round devuelven jakhüwi. */
long aym_floor(int64_t x) { return aym_math_to_long(floor(aym_d(x))); }
long aym_ceil(int64_t x)  { return aym_math_to_long(ceil(aym_d(x))); }
long aym_round(int64_t x) { return aym_math_to_long(round(aym_d(x))); }
