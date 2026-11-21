// Concatenacion de cadenas: s = s + "ab" repetido.
#include <cstdio>
#include <string>
int main() {
    std::string s = "";
    for (long long i = 0; i < 20000; i = i + 1) {
        s = s + "ab";
    }
    std::printf("%lld\n", (long long)s.size());
    return 0;
}
