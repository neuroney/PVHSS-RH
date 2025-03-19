#pragma once
#include "GroupPi2.h"

void PVHSS_TIME_TEST(int msg_num, int degree_f, int cyctimes);

void HSS_TIME_TEST(int msg_num, int degree_f, int cyctimes);


// void DT_TIME_TEST(int model, int depth, int cyctimes);

// // lc I, rc I 
// void Depth2Tree(vec_ZZ &res, vec_ZZ Ip, vec_ZZ Ilc, vec_ZZ Irc, vec_ZZ M1_b, int B, Para param, EK ekb, int &prf_key);

// // lc M, rc I 
// void Depth2Tree_Type1(vec_ZZ &res, vec_ZZ Ip, vec_ZZ Mlc, vec_ZZ Irc, vec_ZZ M1_b, int B, Para param, EK ekb, int &prf_key);

// // lc I, rc M 
// void Depth2Tree_Type2(vec_ZZ &res, vec_ZZ Ip, vec_ZZ Ilc, vec_ZZ Mrc, vec_ZZ M1_b, int B, Para param, EK ekb, int &prf_key);

// // lc M, rc M 
// void Depth2Tree_Type3(vec_ZZ &res, vec_ZZ Ip, vec_ZZ Mlc, vec_ZZ Mrc, vec_ZZ M1_b, int B, Para param, EK ekb, int &prf_key);

// void Eval_DT(bn_t y, PROOF &pi, int B, Para param, EK ekb, int model, int depth, Vec<vec_ZZ> Ibs, Vec<vec_ZZ> Ics);

// void select(ZZ &res, int b, PK pk, EK ekb, Vec<ZZ> ct_idx, vector<Vec<ZZ>> y, int &prf_key);

// void selectTest();