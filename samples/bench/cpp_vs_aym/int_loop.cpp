// Bucle de aritmetica entera: suma modular.
#include <cstdio>
int main() {
    long long s = 0;
    for (long long i = 0; i < 20000000; i = i + 1) {
        s = (s + i * 3 + 7) % 1000003;
    }
    std::printf("%lld\n", s);
    return 0;
}
