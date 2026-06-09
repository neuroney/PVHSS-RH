#include "PiDCRLWE.h"

int main()
{
    return pvhss::rlwe::dc::PVHSS_ACC_TEST(5, 5) ? 0 : 1;
}
