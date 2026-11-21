// Criba de Eratostenes sobre una lista.
#include <cstdio>
#include <vector>
int main() {
    long long n = 2000000;
    std::vector<long long> f;
    for (long long i = 0; i < n; i = i + 1) {
        f.push_back(1);
    }
    f[0] = 0;
    f[1] = 0;
    for (long long i = 2; i * i < n; i = i + 1) {
        if (f[i] == 1) {
            for (long long j = i * i; j < n; j = j + i) {
                f[j] = 0;
            }
        }
    }
    long long c = 0;
    for (long long i = 0; i < n; i = i + 1) {
        c = c + f[i];
    }
    std::printf("%lld\n", c);
    return 0;
}
