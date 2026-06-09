#include "PiOTRLWE.h"

int main()
{
    return pvhss::rlwe::ot::PVHSS_ACC_TEST(5, 5) ? 0 : 1;
}
