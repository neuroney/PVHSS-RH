#pragma once
#include "HSSRLWE.h"

namespace pvhss { namespace rlwe { namespace hss {

void RunGen(int msg_num, int degree_f);

void TimeGen(int msg_num, int degree_f, int cyctimes);

void TimeEnc(int msg_num, int degree_f,  int cyctimes);

void TimeEvalSubalgo(int msg_num, int degree_f,  int cyctimes);

void Time_Eval(int msg_num, int degree_f, int cyctimes);

void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes);

}}} // namespace pvhss::rlwe::hss
