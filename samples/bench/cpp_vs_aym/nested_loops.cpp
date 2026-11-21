// Bucles anidados: acumulador modular sobre i*j.
#include <cstdio>
int main() {
    long long acc = 0;
    for (long long i = 0; i < 3000; i = i + 1) {
        for (long long j = 0; j < 3000; j = j + 1) {
            acc = (acc + i * j) % 1000003;
        }
    }
    std::printf("%lld\n", acc);
    return 0;
}
