// Mapa: insercion y busqueda con claves de texto.
#include <cstdio>
#include <string>
#include <unordered_map>
int main() {
    long long n = 8000;
    std::unordered_map<std::string, long long> m;
    for (long long i = 0; i < n; i = i + 1) {
        m["k" + std::to_string(i)] = i;
    }
    long long s = 0;
    for (long long i = 0; i < n; i = i + 1) {
        s = s + m["k" + std::to_string(i)];
    }
    std::printf("%lld\n", s);
    return 0;
}
