#pragma once
#include "Ped.h"
#include "VHSSElg.h"

typedef HSS_EK PVHSS_EK;
typedef HSS_CT PVHSS_CT;
typedef ZZ PVHSS_SK;

typedef struct
{
    HSS_PK pk;
    CK ck;

    int skLen;
    int vkLen;

    int msg_bits;
    int degree_f;
    int msg_num;
    HSS_CT pk_f; 
} PVHSSPara;

void Setup(PVHSSPara &param, PVHSS_EK &ek0, PVHSS_EK &ek1);
void KeyGen(PVHSSPara &param, PVHSS_SK &sk);
void ProbGen(vector<PVHSS_CT> &Ix, const PVHSSPara &param, const vec_ZZ &x);

void Compute(PROOF &proof, int b, const PVHSSPara &param, const PVHSS_EK &ekb, vector<PVHSS_CT> &Ix, vector<vector<int>> F_TEST);
bool Verify(const PROOF &pi0, const PROOF &pi1, const CK &ck);
void Decode(bn_t y, const PROOF &pi0, const PROOF &pi1, const PVHSS_SK sk);

void PVHSS_ACC_TEST(int msg_num, int degree_f);