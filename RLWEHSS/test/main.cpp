#include "tester.h"

int main(int, char **)
{
    int x1 = 1, x2 = 2, x3 = 3, x4 = 4, x5 = 5;
    int d = 5;  // 指数最大和

    long long result = P_d(x1, x2, x3, x4, x5, d);
    cout << "P_" << d << "(" << x1 << "," << x2 << "," << x3 << "," << x4 << "," << x5 << ") = " << result << endl;

    return 0;
}
