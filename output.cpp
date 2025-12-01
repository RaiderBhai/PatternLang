#include <iostream>
#include <string>
using namespace std;


int main() {
    int i = 0;
    int t4 = 0;
    bool t3 = false;
    bool t2 = false;
    bool t1 = false;
    i = 0;
L1:
    t1 = i < 10;
    if (!(t1)) goto L2;
    t2 = i % 2;
    t3 = t2 == 0;
    if (!(t3)) goto L4;
    cout << i;
    cout << endl;
    goto L4;
L4:
    t4 = i + 1;
    i = t4;
    goto L1;
L2:
    return 0;
}
