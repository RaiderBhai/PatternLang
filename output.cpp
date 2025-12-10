#include <iostream>
#include <string>
#include <cmath>
using namespace std;

int min(int a, int b) { return a < b ? a : b; }

int sq(int x);

int sq(int x) {
    int t1 = 0;
    t1 = x * x;
    return t1;
    return 0;
    endfunc_sq:
    return 0;
}

int main() {
    int n = 0;
    int x = 0;
    bool t5 = false;
    int t4 = 0;
    int t6 = 0;
    int t1 = 0;
    int t3 = 0;
    bool t2 = false;
    func_sq:
    n = 0;
    cin >> n;
    t2 = n > 5;
    if (!(t2)) goto L1;
    t3 = sq(n);
    t1 = t3;
    cout << "square: " << endl;
    cout << t3 << endl;
    cout << endl;
    goto L2;
    L1:
    t4 = min(7, 3);
    t1 = t4;
    cout << "min of 7 and 3: " << endl;
    cout << t4 << endl;
    cout << endl;
    L2:
    int i = 1;
    L3:
    t5 = i <= 5;
    if (!(t5)) goto L4;
    cout << i << endl;
    cout << endl;
    t6 = i + 1;
    i = t6;
    goto L3;
    L4:
    return 0;
}
