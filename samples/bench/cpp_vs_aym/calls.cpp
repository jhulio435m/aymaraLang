// Codigo dominado por llamadas a funcion.
#include <cstdio>
static long long mezcla(long long a, long long b, long long c) {
    return (a * 31 + b * 17 + c) % 1000003;
}
int main() {
    long long acc = 1;
    for (long long i = 0; i < 20000000; i = i + 1) {
        acc = mezcla(acc, i, 7);
    }
    std::printf("%lld\n", acc);
    return 0;
}
