#pragma once
#include "VHSSRLWE.h"

namespace pvhss { namespace rlwe { namespace vhss {

void VHSSRLWE_TIME_TEST(int msg_num, int degree_f, int cyctimes);
bool VHSSRLWE_ACC_TEST(int msg_num, int degree_f);

}}} // namespace pvhss::rlwe::vhss
