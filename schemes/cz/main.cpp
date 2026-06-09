#include <iostream>
#include <gmp.h>
extern "C" {
#include <relic/relic.h>
}
#include <cstring>
#include "function.h"

int main() {
    std::vector<int> deg_list = {5, 10, 15};
    int num_data = 5;
    for (size_t i = 0; i < deg_list.size(); i++) {
        std::cout << "Degree: " << deg_list[i] << "\n";
        EvalPoly(deg_list[i], num_data);
    }
}
