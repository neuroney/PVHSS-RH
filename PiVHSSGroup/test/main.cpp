#include "tester.h"

int main(int, char **)
{ 
    int msg_num = 5;
    int cyctimes = 5;
    vector<int> degree_f = {5,10,15};
   
    for (int i = 0; i < degree_f.size(); ++i)
    {    
        cout << "degree_f: " << degree_f[i] << endl;
        VHSSElg_TIME_TEST(msg_num, degree_f[i] ,cyctimes);
    }
    return 0;
}
