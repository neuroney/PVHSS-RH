#include "tester.h"

int main(int, char **)
{ 
    int degree_f = 16;
    int msg_num = 5;
    int cyctimes = 5;
    PVHSS_ACC_TEST(msg_num, 1);
    for (int deg = 1; deg < degree_f; deg++)
    {   
        //PVHSS_TIME_TEST(msg_num, deg, cyctimes);
    }
    
    return 0;
}
