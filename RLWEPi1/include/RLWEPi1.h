#pragma once
#include "Ped.h"
#include "VHSSRLWE.h"

typedef ZZ PVHSS_SK;
typedef vec_ZZ_pX PVHSS_CT;
typedef vec_ZZ_pX PVHSS_MV;

typedef ZZ PVHSS_SK;

typedef struct
{
    PKE_Para pkePara;
    VHSS_Para vhssPara;
    ZZ sk_alpha;
    CK ck;

    int skLen;
    int vkLen;
    vec_ZZ_pX pk_f; 
} PVHSSPara;

void Setup(PVHSSPara &param, vec_ZZ_pX &pkePk, int msg_num, int degree_f);
void KeyGen(PVHSSPara &param, PVHSS_SK &sk, ZZ_pXModulus modulus, vec_ZZ_pX pkePk);
void ProbGen(vector<PVHSS_CT> &Ix, const PVHSSPara &param, const vec_ZZ &x, ZZ_pXModulus modulus, vec_ZZ_pX pkePk);

void Prove(PROOF &proof, int b, const PVHSS_MV& y_b, const PVHSS_MV& Y_b, const PVHSSPara & param);
void Compute(PROOF &proof, int b, const PVHSSPara &param, const vec_ZZ_pX &ek1, const vec_ZZ_pX &ek2, vector<PVHSS_CT> &Ix, ZZ_pXModulus modulus, const vec_ZZ_pX &M1, vec_ZZ_pX &M2, vector<vector<int>> F_TEST);
bool Verify(const PROOF &pi0, const PROOF &pi1, const CK &ck);
void Decode(ZZ& y, const PROOF &pi0, const PROOF &pi1, const PVHSS_SK sk);