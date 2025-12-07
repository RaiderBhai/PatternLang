#include <iostream>
#include <string>
#include <cmath>
using namespace std;

string repeat(string c, int times) { string s; for (int i = 0; i < times; ++i) s += c; return s; }
void pyramid(int height) { for (int i = 1; i <= height; ++i) { for (int j = 0; j < height - i; ++j) cout << ' '; for (int j = 0; j < 2 * i - 1; ++j) cout << '*'; cout << endl; } }
void diamond(int height) { int n = height; for (int i = 1; i <= n; ++i) { for (int j = 0; j < n - i; ++j) cout << ' '; for (int j = 0; j < 2 * i - 1; ++j) cout << '*'; cout << endl; } for (int i = n - 1; i >= 1; --i) { for (int j = 0; j < n - i; ++j) cout << ' '; for (int j = 0; j < 2 * i - 1; ++j) cout << '*'; cout << endl; } }
void line(string c, int width) { for (int i = 0; i < width; ++i) cout << c; cout << endl; }
void box(string c, int width, int height) { for (int i = 0; i < height; ++i) { for (int j = 0; j < width; ++j) cout << c; cout << endl; } }
void stairs(int height, string c) { for (int i = 1; i <= height; ++i) { for (int j = 0; j < i; ++j) cout << c; cout << endl; } }
int max(int a, int b) { return a > b ? a : b; }
int min(int a, int b) { return a < b ? a : b; }
int abs(int x) { return x < 0 ? -x : x; }
int pow(int a, int b) { return static_cast<int>(std::pow(a, b)); }
int sqrt(int n) { return static_cast<int>(std::sqrt(n)); }
int rangeSum(int n) { int s = 0; for (int i = 1; i <= n; ++i) s += i; return s; }
void factor(int n) { for (int i = 2; i <= n; ++i) { while (n % i == 0) { cout << i << ' '; n /= i; } } cout << endl; }
bool isPrime(int n) { if (n <= 1) return false; for (int i = 2; i * i <= n; ++i) if (n % i == 0) return false; return true; }
void table(int n) { for (int i = 1; i <= n; ++i) { for (int j = 1; j <= n; ++j) cout << i * j << '	'; cout << endl; } }
void patternMultiply(int a, int b) { for (int i = 0; i < a; ++i) { for (int j = 0; j < b; ++j) cout << '*'; cout << endl; } }

int fact(int n);

int fact(int n) {
    int t3 = 0;
    int t4 = 0;
    int t2 = 0;
    bool t1 = false;
    t1 = n <= 1;
    if (!(t1)) goto L2;
    return 1;
    goto L2;
    L2:
    t2 = n - 1;
    t3 = fact(t2);
    t4 = n * t3;
    return t4;
    return 0;
    endfunc_fact:
    return 0;
}

int main() {
    int n = 0;
    cout << fact(5) << endl;
    cout << endl;
    return 0;
}
