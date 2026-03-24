#include <limits.h>
#include <math.h>

static long aym_math_to_long(double value) {
    if (!isfinite(value)) return 0;
    if (value > (double)LONG_MAX) return LONG_MAX;
    if (value < (double)LONG_MIN) return LONG_MIN;
    return (long)llround(value);
}

long aym_sin(long x) {
    return aym_math_to_long(sin((double)x));
}

long aym_cos(long x) {
    return aym_math_to_long(cos((double)x));
}

long aym_tan(long x) {
    return aym_math_to_long(tan((double)x));
}

long aym_asin(long x) {
    return aym_math_to_long(asin((double)x));
}

long aym_acos(long x) {
    return aym_math_to_long(acos((double)x));
}

long aym_atan(long x) {
    return aym_math_to_long(atan((double)x));
}

long aym_sqrt(long x) {
    return aym_math_to_long(sqrt((double)x));
}

long aym_pow(long base, long exponent) {
    return aym_math_to_long(pow((double)base, (double)exponent));
}

long aym_exp(long x) {
    return aym_math_to_long(exp((double)x));
}

long aym_log(long x) {
    return aym_math_to_long(log((double)x));
}

long aym_log10(long x) {
    return aym_math_to_long(log10((double)x));
}

long aym_floor(long x) {
    return aym_math_to_long(floor((double)x));
}

long aym_ceil(long x) {
    return aym_math_to_long(ceil((double)x));
}

long aym_round(long x) {
    return aym_math_to_long(round((double)x));
}

long aym_fabs(long x) {
    return aym_math_to_long(fabs((double)x));
}
