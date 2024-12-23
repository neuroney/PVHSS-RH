#pragma once
#include "CAPSNARK.h"
#include "VHSSElg.h"
typedef struct
{
    HSS_PK pk;
    CK ck;

    int skLen;
    int vkLen;

    int msg_bits;
    int degree_f;
    int msg_num;
} PVHSSPara;

typedef HSS_EK PVHSS_EK;
typedef HSS_CT PVHSS_CT;

void KeyGen(PVHSSPara &param, PVHSS_EK &ek0, PVHSS_EK &ek1);
void ProbGen(vector<PVHSS_CT> &Ix, const PVHSSPara &param, const vec_ZZ &x);

void evaluate(bn_t y, PROOF &proof, int b, const PVHSSPara &param, const PVHSS_EK &ekb, vector<PVHSS_CT> &Ix, vector<vector<int>> F_TEST);
bool verify(const PROOF &pi0, const PROOF &pi1, const CK &ck);
bool verify(bn_t y0, bn_t y1, const PROOF &pi0, const PROOF &pi1, const CK &ck);
void dec(bn_t y, bn_t y0, bn_t y1);

void PVHSS_ACC_TEST(int msg_num, int degree_f);