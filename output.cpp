#include <iostream>
#include <string>
using namespace std;

int fact(int n);

int fact(int n) {
    int t4 = 0;
    int t3 = 0;
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
    int t5 = 0;
    t5 = fact(5);
    cout << t5;
    cout << endl;
    return 0;
}
