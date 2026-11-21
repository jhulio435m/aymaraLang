// Llamadas a metodos de objeto.
#include <cstdio>
class Contador {
    long long n;
public:
    Contador() { n = 0; }
    void suma(long long x) { n = (n + x) % 1000003; }
    long long ver() { return n; }
};
int main() {
    Contador* c = new Contador();
    for (long long i = 0; i < 2000000; i = i + 1) {
        c->suma(i);
    }
    std::printf("%lld\n", c->ver());
    delete c;
    return 0;
}
