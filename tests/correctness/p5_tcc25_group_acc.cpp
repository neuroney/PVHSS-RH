#include "Tcc25Group.h"

int main()
{
    return pvhss::group::tcc25::Tcc25_ACC_TEST(5, 5) ? 0 : 1;
}
