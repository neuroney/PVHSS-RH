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
     cout << "\n\n**************Multivariate polynomial evaluation (Section V-B in the paper)**************"
          << "\n";
     // the degree of the polynomial, and it set to 2,4,...,20 in the experiments in our paper.
     vector<int> deg_list = {5,10,15};
     int num_data = 5;
     for (int i = 0; i <= deg_list.size(); i += 1)
     {
          cout << "Degree: " << deg_list[i] << endl;
          Eval_Poly(deg_list[i], num_data);
     }
}
