#include "tester.h"

int main(int, char **)
{ 
    int degree_f = 16;
    int msg_num = 5;
    int cyctimes = 5;
    for (int deg = 1; deg < 2; deg++)
    {    
        //PVHSS_ACC_TEST(msg_num, deg);
        PVHSS_TIME_TEST(msg_num, deg, cyctimes);
    }
    
    return 0;
}
