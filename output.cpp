#include <iostream>
#include <string>
using namespace std;

int inc(int v);
int twice(int v);
int add(int x, int y);

int inc(int v) {
    int t2 = 0;
    t2 = add(v, 1);
    return t2;
    return 0;
    endfunc_inc:
    return 0;
}

int twice(int v) {
    int t4 = 0;
    int t3 = 0;
    t3 = inc(v);
    t4 = inc(t3);
    return t4;
    return 0;
    endfunc_twice:
    return 0;
}

int add(int x, int y) {
    int t1 = 0;
    t1 = x + y;
    return t1;
    return 0;
    endfunc_add:
    return 0;
}

int main() {
    int y = 0;
    int v = 0;
    int x = 0;
    int t5 = 0;
    t5 = twice(5);
    cout << t5;
    cout << endl;
    return 0;
}
