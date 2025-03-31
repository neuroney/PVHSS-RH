#include <iostream>
#include <gmp.h>
// #include <gmpxx.h>
extern "C"
{
#include <relic/relic.h>
}
#include <cstring>
#include "function.h"
using namespace std;

int main()
{
     vector<int> deg_list = {5,10,15};
     int num_data = 5;
     for (int i = 0; i < deg_list.size(); i += 1)
     {
          cout << "Degree: " << deg_list[i] << endl;
          Eval_Poly(deg_list[i], num_data);
     }
}
