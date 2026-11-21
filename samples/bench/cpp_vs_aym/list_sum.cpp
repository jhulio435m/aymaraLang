// Llenado de lista con push y suma por indice (30 pasadas).
#include <cstdio>
#include <vector>
int main() {
    long long n = 1000000;
    std::vector<long long> xs;
    for (long long i = 0; i < n; i = i + 1) {
        xs.push_back(i % 100);
    }
    long long s = 0;
    for (long long p = 0; p < 30; p = p + 1) {
        for (long long i = 0; i < n; i = i + 1) {
            s = s + xs[i];
        }
    }
    std::printf("%lld\n", s);
    return 0;
}
