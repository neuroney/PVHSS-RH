#pragma once
#include "Ped.h"
#include "VHSSElg.h"

typedef VhssElgamalEk PVHSSElg1_EK;
typedef VhssElgamalCt PVHSSElg1_CT;
typedef ZZ PVHSSElg1_SK;

typedef struct
{
    VhssElgamalPk pk;
    CK ck;

    int skLen;
    int vkLen;

    int msg_bits;
    int degree_f;
    int msg_num;
    VhssElgamalCt pk_f;
} PVHSSElg1_Para;

void PVHSSElg1_Setup(PVHSSElg1_Para &param, PVHSSElg1_EK &ek0, PVHSSElg1_EK &ek1);
void PVHSSElg1_KeyGen(PVHSSElg1_Para &param, PVHSSElg1_SK &sk, bn_t ekp0, bn_t ekp1);
void PVHSSElg1_ProbGen(vector<PVHSSElg1_CT> &Ix, const PVHSSElg1_Para &param, const vec_ZZ &x);

void PVHSSElg1_Compute(PROOF &proof, int b, const PVHSSElg1_Para &param, const PVHSSElg1_EK &ekb, vector<PVHSSElg1_CT> &Ix, vector<vector<int>> F_TEST, bn_t ekpb);
bool PVHSSElg1_Verify(const PROOF &pi0, const PROOF &pi1, const CK &ck);
void PVHSSElg1_Decode(ZZ& y, const PROOF &pi0, const PROOF &pi1, const PVHSSElg1_SK sk);

bool PVHSSElg1_ACC_TEST(int msg_num, int degree_f);
