#pragma once
#include "Ped.h"
#include "RLWEVHSS.h"

using NTL::vec_ZZ;
using NTL::vec_ZZ_pX;
using NTL::ZZ;
using NTL::ZZ_p;
using NTL::ZZ_pX;
using NTL::ZZ_pXModulus;

namespace pvhss { namespace rlwe { namespace ot {

using pvhss::rlwe::common::Data;
using pvhss::rlwe::common::PKE_Para;
using pvhss::rlwe::common::PKEWorkspace;
using pvhss::rlwe::common::VHSS_Para;
using pvhss::rlwe::common::EncodeBinaryPolynomial;
using pvhss::rlwe::common::GenerateData;
using pvhss::rlwe::common::HssAddMemory;
using pvhss::rlwe::common::HssAddMemoryInPlace;
using pvhss::rlwe::common::HssConvertInput;
using pvhss::rlwe::common::HssEvaluateMPE;
using pvhss::rlwe::common::HssGen;
using pvhss::rlwe::common::HssOutputCoeff;
using pvhss::rlwe::common::HssOutputPolyAtTwo;
using pvhss::rlwe::common::PKE_DDec;
using pvhss::rlwe::common::PKE_Enc;
using pvhss::rlwe::common::PKE_Gen;
using pvhss::rlwe::common::PKE_OKDM;
using pvhss::rlwe::common::SetParams;
using pvhss::rlwe::common::VHSS_Enc;
using pvhss::rlwe::common::VHSS_Mult;

typedef ZZ PVHSS_SK;
typedef vec_ZZ_pX PVHSS_CT;
typedef vec_ZZ_pX PVHSS_MV;

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
void KeyGen(PVHSSPara &param, PVHSS_SK &sk, ZZ_pXModulus modulus, vec_ZZ_pX pkePk, bn_t ekp0, bn_t ekp1);
void ProbGen(vector<PVHSS_CT> &Ix, const PVHSSPara &param, const vec_ZZ_pX &x, ZZ_pXModulus modulus, vec_ZZ_pX pkePk);

void Prove(PROOF &proof, int b, const PVHSS_MV& y_b, const PVHSS_MV& Y_b, const PVHSSPara & param, bn_t ekpb);
void Compute(PROOF &proof, int b, const PVHSSPara &param, const vec_ZZ_pX &ek1, const vec_ZZ_pX &ek2, vector<PVHSS_CT> &Ix, ZZ_pXModulus modulus, const vec_ZZ_pX &M1, vec_ZZ_pX &M2, vector<vector<int>> F_TEST, bn_t ekpb);
bool Verify(const PROOF &pi0, const PROOF &pi1, const CK &ck);
void Decode(ZZ& y, const PROOF &pi0, const PROOF &pi1, const PVHSS_SK sk);
bool PVHSS_ACC_TEST(int msg_num, int degree_f);

}}} // namespace pvhss::rlwe::ot
