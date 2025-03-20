#pragma once
#include "HSSRLWE.h"

void Run_Gen(int msg_num, int degree_f);

void Time_Gen(int msg_num, int degree_f, int cyctimes);

void Time_Enc(int msg_num, int degree_f,  int cyctimes);

void Time_Eval_Subalgo(int msg_num, int degree_f,  int cyctimes);

void Time_Eval(int msg_num, int degree_f, int cyctimes);
