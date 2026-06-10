#include "tester.h"

int main()
{
    return pvhss::rlwe::vhss::VHSSRLWE_ACC_TEST(5, 5) ? 0 : 1;
}
