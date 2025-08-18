#pragma once
#include "DecPed.h"
#include "VHSSElg.h"

typedef VHSSElg_EK PVHSSElg2_EK;
typedef VHSSElg_CT PVHSSElg2_CT;
typedef bgn_t PVHSSElg2_SK;
typedef g2_t PVHSSElg2_VK;
typedef struct
{
    VHSSElg_PK pk;
    CK ck;
    PVHSSElg2_VK vk;
    PVHSSElg2_SK sk;

    int skLen;
    int vkLen;

    int msg_bits;
    int degree_f;
    int msg_num;
    VHSSElg_CT pk_f; 
    ZZ n_out;
} PVHSSElg2_Para;

void PVHSSElg2_Setup(PVHSSElg2_Para &param, PVHSSElg2_EK &ek0, PVHSSElg2_EK &ek1, PVHSSElg2_SK &sk);
void PVHSSElg2_KeyGen(PVHSSElg2_Para &param, PVHSSElg2_SK &sk);
void PVHSSElg2_ProbGen(vector<PVHSSElg2_CT> &Ix, const PVHSSElg2_Para &param, const vec_ZZ &x);

void PVHSSElg2_Compute(PROOF &proof, int b, const PVHSSElg2_Para &param, const PVHSSElg2_EK &ekb, vector<PVHSSElg2_CT> &Ix, vector<vector<int>> F_TEST);
bool PVHSSElg2_Verify(const PROOF &pi0, const PROOF &pi1, const PVHSSElg2_Para &param);
void PVHSSElg2_Decode(dig_t &y, const PROOF &pi0, const PROOF &pi1, const PVHSSElg2_SK sk);


void PVHSSElg2_Prove(PROOF &pi, int b, const ZZ &yb, const ZZ &Yb, const PVHSSElg2_Para &param);

void PVHSSElg2_ACC_TEST(int msg_num, int degree_f);