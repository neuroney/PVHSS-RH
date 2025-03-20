#include "tester.h"

int main(int, char **)
{   
    Vec<ZZ> x;
    x.SetLength(5);
    x[0] = conv<ZZ>(1);
    x[1] = conv<ZZ>(2);
    x[2] = conv<ZZ>(3);
    x[3] = conv<ZZ>(4);
    x[4] = conv<ZZ>(5);
    int d =5;  // 指数最大和

    ZZ result = P_d2(x, d);
    cout << result << endl;

    return 0;
}
